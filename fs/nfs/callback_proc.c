/*
 * fikus/fs/nfs/callback_proc.c
 *
 * Copyright (C) 2004 Trond Myklebust
 *
 * NFSv4 callback procedures
 */
#include <fikus/nfs4.h>
#include <fikus/nfs_fs.h>
#include <fikus/slab.h>
#include <fikus/rcupdate.h>
#include "nfs4_fs.h"
#include "callback.h"
#include "delegation.h"
#include "internal.h"
#include "pnfs.h"
#include "nfs4session.h"
#include "nfs4trace.h"

#ifdef NFS_DEBUG
#define NFSDBG_FACILITY NFSDBG_CALLBACK
#endif

__be32 nfs4_callback_getattr(struct cb_getattrargs *args,
			     struct cb_getattrres *res,
			     struct cb_process_state *cps)
{
	struct nfs_delegation *delegation;
	struct nfs_inode *nfsi;
	struct inode *inode;

	res->status = htonl(NFS4ERR_OP_NOT_IN_SESSION);
	if (!cps->clp) /* Always set for v4.0. Set in cb_sequence for v4.1 */
		goto out;

	res->bitmap[0] = res->bitmap[1] = 0;
	res->status = htonl(NFS4ERR_BADHANDLE);

	dprintk_rcu("NFS: GETATTR callback request from %s\n",
		rpc_peeraddr2str(cps->clp->cl_rpcclient, RPC_DISPLAY_ADDR));

	inode = nfs_delegation_find_inode(cps->clp, &args->fh);
	if (inode == NULL)
		goto out;
	nfsi = NFS_I(inode);
	rcu_read_lock();
	delegation = rcu_dereference(nfsi->delegation);
	if (delegation == NULL || (delegation->type & FMODE_WRITE) == 0)
		goto out_iput;
	res->size = i_size_read(inode);
	res->change_attr = delegation->change_attr;
	if (nfsi->npages != 0)
		res->change_attr++;
	res->ctime = inode->i_ctime;
	res->mtime = inode->i_mtime;
	res->bitmap[0] = (FATTR4_WORD0_CHANGE|FATTR4_WORD0_SIZE) &
		args->bitmap[0];
	res->bitmap[1] = (FATTR4_WORD1_TIME_METADATA|FATTR4_WORD1_TIME_MODIFY) &
		args->bitmap[1];
	res->status = 0;
out_iput:
	rcu_read_unlock();
	iput(inode);
out:
	dprintk("%s: exit with status = %d\n", __func__, ntohl(res->status));
	return res->status;
}

__be32 nfs4_callback_recall(struct cb_recallargs *args, void *dummy,
			    struct cb_process_state *cps)
{
	struct inode *inode;
	__be32 res;
	
	res = htonl(NFS4ERR_OP_NOT_IN_SESSION);
	if (!cps->clp) /* Always set for v4.0. Set in cb_sequence for v4.1 */
		goto out;

	dprintk_rcu("NFS: RECALL callback request from %s\n",
		rpc_peeraddr2str(cps->clp->cl_rpcclient, RPC_DISPLAY_ADDR));

	res = htonl(NFS4ERR_BADHANDLE);
	inode = nfs_delegation_find_inode(cps->clp, &args->fh);
	if (inode == NULL)
		goto out;
	/* Set up a helper thread to actually return the delegation */
	switch (nfs_async_inode_return_delegation(inode, &args->stateid)) {
	case 0:
		res = 0;
		break;
	case -ENOENT:
		res = htonl(NFS4ERR_BAD_STATEID);
		break;
	default:
		res = htonl(NFS4ERR_RESOURCE);
	}
	trace_nfs4_recall_delegation(inode, -ntohl(res));
	iput(inode);
out:
	dprintk("%s: exit with status = %d\n", __func__, ntohl(res));
	return res;
}

#if defined(CONFIG_NFS_V4_1)

/*
 * Lookup a layout by filehandle.
 *
 * Note: gets a refcount on the layout hdr and on its respective inode.
 * Caller must put the layout hdr and the inode.
 *
 * TODO: keep track of all layouts (and delegations) in a hash table
 * hashed by filehandle.
 */
static struct pnfs_layout_hdr * get_layout_by_fh_locked(struct nfs_client *clp, struct nfs_fh *fh)
{
	struct nfs_server *server;
	struct inode *ino;
	struct pnfs_layout_hdr *lo;

	list_for_each_entry_rcu(server, &clp->cl_superblocks, client_link) {
		list_for_each_entry(lo, &server->layouts, plh_layouts) {
			if (nfs_compare_fh(fh, &NFS_I(lo->plh_inode)->fh))
				continue;
			ino = igrab(lo->plh_inode);
			if (!ino)
				continue;
			spin_lock(&ino->i_lock);
			/* Is this layout in the process of being freed? */
			if (NFS_I(ino)->layout != lo) {
				spin_unlock(&ino->i_lock);
				iput(ino);
				continue;
			}
			pnfs_get_layout_hdr(lo);
			spin_unlock(&ino->i_lock);
			return lo;
		}
	}

	return NULL;
}

static struct pnfs_layout_hdr * get_layout_by_fh(struct nfs_client *clp, struct nfs_fh *fh)
{
	struct pnfs_layout_hdr *lo;

	spin_lock(&clp->cl_lock);
	rcu_read_lock();
	lo = get_layout_by_fh_locked(clp, fh);
	rcu_read_unlock();
	spin_unlock(&clp->cl_lock);

	return lo;
}

static u32 initiate_file_draining(struct nfs_client *clp,
				  struct cb_layoutrecallargs *args)
{
	struct inode *ino;
	struct pnfs_layout_hdr *lo;
	u32 rv = NFS4ERR_NOMATCHING_LAYOUT;
	LIST_HEAD(free_me_list);

	lo = get_layout_by_fh(clp, &args->cbl_fh);
	if (!lo)
		return NFS4ERR_NOMATCHING_LAYOUT;

	ino = lo->plh_inode;
	spin_lock(&ino->i_lock);
	if (test_bit(NFS_LAYOUT_BULK_RECALL, &lo->plh_flags) ||
	    pnfs_mark_matching_lsegs_invalid(lo, &free_me_list,
					&args->cbl_range))
		rv = NFS4ERR_DELAY;
	else
		rv = NFS4ERR_NOMATCHING_LAYOUT;
	pnfs_set_layout_stateid(lo, &args->cbl_stateid, true);
	spin_unlock(&ino->i_lock);
	pnfs_free_lseg_list(&free_me_list);
	pnfs_put_layout_hdr(lo);
	iput(ino);
	return rv;
}

static u32 initiate_bulk_draining(struct nfs_client *clp,
				  struct cb_layoutrecallargs *args)
{
	int stat;

	if (args->cbl_recall_type == RETURN_FSID)
		stat = pnfs_destroy_layouts_byfsid(clp, &args->cbl_fsid, true);
	else
		stat = pnfs_destroy_layouts_byclid(clp, true);
	if (stat != 0)
		return NFS4ERR_DELAY;
	return NFS4ERR_NOMATCHING_LAYOUT;
}

static u32 do_callback_layoutrecall(struct nfs_client *clp,
				    struct cb_layoutrecallargs *args)
{
	u32 res;

	dprintk("%s enter, type=%i\n", __func__, args->cbl_recall_type);
	if (args->cbl_recall_type == RETURN_FILE)
		res = initiate_file_draining(clp, args);
	else
		res = initiate_bulk_draining(clp, args);
	dprintk("%s returning %i\n", __func__, res);
	return res;

}

__be32 nfs4_callback_layoutrecall(struct cb_layoutrecallargs *args,
				  void *dummy, struct cb_process_state *cps)
{
	u32 res;

	dprintk("%s: -->\n", __func__);

	if (cps->clp)
		res = do_callback_layoutrecall(cps->clp, args);
	else
		res = NFS4ERR_OP_NOT_IN_SESSION;

	dprintk("%s: exit with status = %d\n", __func__, res);
	return cpu_to_be32(res);
}

static void pnfs_recall_all_layouts(struct nfs_client *clp)
{
	struct cb_layoutrecallargs args;

	/* Pretend we got a CB_LAYOUTRECALL(ALL) */
	memset(&args, 0, sizeof(args));
	args.cbl_recall_type = RETURN_ALL;
	/* FIXME we ignore errors, what should we do? */
	do_callback_layoutrecall(clp, &args);
}

__be32 nfs4_callback_devicenotify(struct cb_devicenotifyargs *args,
				  void *dummy, struct cb_process_state *cps)
{
	int i;
	__be32 res = 0;
	struct nfs_client *clp = cps->clp;
	struct nfs_server *server = NULL;

	dprintk("%s: -->\n", __func__);

	if (!clp) {
		res = cpu_to_be32(NFS4ERR_OP_NOT_IN_SESSION);
		goto out;
	}

	for (i = 0; i < args->ndevs; i++) {
		struct cb_devicenotifyitem *dev = &args->devs[i];

		if (!server ||
		    server->pnfs_curr_ld->id != dev->cbd_layout_type) {
			rcu_read_lock();
			list_for_each_entry_rcu(server, &clp->cl_superblocks, client_link)
				if (server->pnfs_curr_ld &&
				    server->pnfs_curr_ld->id == dev->cbd_layout_type) {
					rcu_read_unlock();
					goto found;
				}
			rcu_read_unlock();
			dprintk("%s: layout type %u not found\n",
				__func__, dev->cbd_layout_type);
			continue;
		}

	found:
		if (dev->cbd_notify_type == NOTIFY_DEVICEID4_CHANGE)
			dprintk("%s: NOTIFY_DEVICEID4_CHANGE not supported, "
				"deleting instead\n", __func__);
		nfs4_delete_deviceid(server->pnfs_curr_ld, clp, &dev->cbd_dev_id);
	}

out:
	kfree(args->devs);
	dprintk("%s: exit with status = %u\n",
		__func__, be32_to_cpu(res));
	return res;
}

/*
 * Validate the sequenceID sent by the server.
 * Return success if the sequenceID is one more than what we last saw on
 * this slot, accounting for wraparound.  Increments the slot's sequence.
 *
 * We don't yet implement a duplicate request cache, instead we set the
 * back channel ca_maxresponsesize_cached to zero. This is OK for now
 * since we only currently implement idempotent callbacks anyway.
 *
 * We have a single slot backchannel at this time, so we don't bother
 * checking the used_slots bit array on the table.  The lower layer guarantees
 * a single outstanding callback request at a time.
 */
static __be32
validate_seqid(struct nfs4_slot_table *tbl, struct cb_sequenceargs * args)
{
	struct nfs4_slot *slot;

	dprintk("%s enter. slotid %u seqid %u\n",
		__func__, args->csa_slotid, args->csa_sequenceid);

	if (args->csa_slotid >= NFS41_BC_MAX_CALLBACKS)
		return htonl(NFS4ERR_BADSLOT);

	slot = tbl->slots + args->csa_slotid;
	dprintk("%s slot table seqid: %u\n", __func__, slot->seq_nr);

	/* Normal */
	if (likely(args->csa_sequenceid == slot->seq_nr + 1)) {
		slot->seq_nr++;
		goto out_ok;
	}

	/* Replay */
	if (args->csa_sequenceid == slot->seq_nr) {
		dprintk("%s seqid %u is a replay\n",
			__func__, args->csa_sequenceid);
		/* Signal process_op to set this error on next op */
		if (args->csa_cachethis == 0)
			return htonl(NFS4ERR_RETRY_UNCACHED_REP);

		/* The ca_maxresponsesize_cached is 0 with no DRC */
		else if (args->csa_cachethis == 1)
			return htonl(NFS4ERR_REP_TOO_BIG_TO_CACHE);
	}

	/* Wraparound */
	if (args->csa_sequenceid == 1 && (slot->seq_nr + 1) == 0) {
		slot->seq_nr = 1;
		goto out_ok;
	}

	/* Misordered request */
	return htonl(NFS4ERR_SEQ_MISORDERED);
out_ok:
	tbl->highest_used_slotid = args->csa_slotid;
	return htonl(NFS4_OK);
}

/*
 * For each referring call triple, check the session's slot table for
 * a match.  If the slot is in use and the sequence numbers match, the
 * client is still waiting for a response to the original request.
 */
static bool referring_call_exists(struct nfs_client *clp,
				  uint32_t nrclists,
				  struct referring_call_list *rclists)
{
	bool status = 0;
	int i, j;
	struct nfs4_session *session;
	struct nfs4_slot_table *tbl;
	struct referring_call_list *rclist;
	struct referring_call *ref;

	/*
	 * XXX When client trunking is implemented, this becomes
	 * a session lookup from within the loop
	 */
	session = clp->cl_session;
	tbl = &session->fc_slot_table;

	for (i = 0; i < nrclists; i++) {
		rclist = &rclists[i];
		if (memcmp(session->sess_id.data,
			   rclist->rcl_sessionid.data,
			   NFS4_MAX_SESSIONID_LEN) != 0)
			continue;

		for (j = 0; j < rclist->rcl_nrefcalls; j++) {
			ref = &rclist->rcl_refcalls[j];

			dprintk("%s: sessionid %x:%x:%x:%x sequenceid %u "
				"slotid %u\n", __func__,
				((u32 *)&rclist->rcl_sessionid.data)[0],
				((u32 *)&rclist->rcl_sessionid.data)[1],
				((u32 *)&rclist->rcl_sessionid.data)[2],
				((u32 *)&rclist->rcl_sessionid.data)[3],
				ref->rc_sequenceid, ref->rc_slotid);

			spin_lock(&tbl->slot_tbl_lock);
			status = (test_bit(ref->rc_slotid, tbl->used_slots) &&
				  tbl->slots[ref->rc_slotid].seq_nr ==
					ref->rc_sequenceid);
			spin_unlock(&tbl->slot_tbl_lock);
			if (status)
				goto out;
		}
	}

out:
	return status;
}

__be32 nfs4_callback_sequence(struct cb_sequenceargs *args,
			      struct cb_sequenceres *res,
			      struct cb_process_state *cps)
{
	struct nfs4_slot_table *tbl;
	struct nfs_client *clp;
	int i;
	__be32 status = htonl(NFS4ERR_BADSESSION);

	clp = nfs4_find_client_sessionid(cps->net, args->csa_addr,
					 &args->csa_sessionid, cps->minorversion);
	if (clp == NULL)
		goto out;

	tbl = &clp->cl_session->bc_slot_table;

	spin_lock(&tbl->slot_tbl_lock);
	/* state manager is resetting the session */
	if (test_bit(NFS4_SLOT_TBL_DRAINING, &tbl->slot_tbl_state)) {
		spin_unlock(&tbl->slot_tbl_lock);
		status = htonl(NFS4ERR_DELAY);
		/* Return NFS4ERR_BADSESSION if we're draining the session
		 * in order to reset it.
		 */
		if (test_bit(NFS4CLNT_SESSION_RESET, &clp->cl_state))
			status = htonl(NFS4ERR_BADSESSION);
		goto out;
	}

	status = validate_seqid(&clp->cl_session->bc_slot_table, args);
	spin_unlock(&tbl->slot_tbl_lock);
	if (status)
		goto out;

	cps->slotid = args->csa_slotid;

	/*
	 * Check for pending referring calls.  If a match is found, a
	 * related callback was received before the response to the original
	 * call.
	 */
	if (referring_call_exists(clp, args->csa_nrclists, args->csa_rclists)) {
		status = htonl(NFS4ERR_DELAY);
		goto out;
	}

	memcpy(&res->csr_sessionid, &args->csa_sessionid,
	       sizeof(res->csr_sessionid));
	res->csr_sequenceid = args->csa_sequenceid;
	res->csr_slotid = args->csa_slotid;
	res->csr_highestslotid = NFS41_BC_MAX_CALLBACKS - 1;
	res->csr_target_highestslotid = NFS41_BC_MAX_CALLBACKS - 1;

out:
	cps->clp = clp; /* put in nfs4_callback_compound */
	for (i = 0; i < args->csa_nrclists; i++)
		kfree(args->csa_rclists[i].rcl_refcalls);
	kfree(args->csa_rclists);

	if (status == htonl(NFS4ERR_RETRY_UNCACHED_REP)) {
		cps->drc_status = status;
		status = 0;
	} else
		res->csr_status = status;

	trace_nfs4_cb_sequence(args, res, status);
	dprintk("%s: exit with status = %d res->csr_status %d\n", __func__,
		ntohl(status), ntohl(res->csr_status));
	return status;
}

static bool
validate_bitmap_values(unsigned long mask)
{
	return (mask & ~RCA4_TYPE_MASK_ALL) == 0;
}

__be32 nfs4_callback_recallany(struct cb_recallanyargs *args, void *dummy,
			       struct cb_process_state *cps)
{
	__be32 status;
	fmode_t flags = 0;

	status = cpu_to_be32(NFS4ERR_OP_NOT_IN_SESSION);
	if (!cps->clp) /* set in cb_sequence */
		goto out;

	dprintk_rcu("NFS: RECALL_ANY callback request from %s\n",
		rpc_peeraddr2str(cps->clp->cl_rpcclient, RPC_DISPLAY_ADDR));

	status = cpu_to_be32(NFS4ERR_INVAL);
	if (!validate_bitmap_values(args->craa_type_mask))
		goto out;

	status = cpu_to_be32(NFS4_OK);
	if (test_bit(RCA4_TYPE_MASK_RDATA_DLG, (const unsigned long *)
		     &args->craa_type_mask))
		flags = FMODE_READ;
	if (test_bit(RCA4_TYPE_MASK_WDATA_DLG, (const unsigned long *)
		     &args->craa_type_mask))
		flags |= FMODE_WRITE;
	if (test_bit(RCA4_TYPE_MASK_FILE_LAYOUT, (const unsigned long *)
		     &args->craa_type_mask))
		pnfs_recall_all_layouts(cps->clp);
	if (flags)
		nfs_expire_unused_delegation_types(cps->clp, flags);
out:
	dprintk("%s: exit with status = %d\n", __func__, ntohl(status));
	return status;
}

/* Reduce the fore channel's max_slots to the target value */
__be32 nfs4_callback_recallslot(struct cb_recallslotargs *args, void *dummy,
				struct cb_process_state *cps)
{
	struct nfs4_slot_table *fc_tbl;
	__be32 status;

	status = htonl(NFS4ERR_OP_NOT_IN_SESSION);
	if (!cps->clp) /* set in cb_sequence */
		goto out;

	dprintk_rcu("NFS: CB_RECALL_SLOT request from %s target highest slotid %u\n",
		rpc_peeraddr2str(cps->clp->cl_rpcclient, RPC_DISPLAY_ADDR),
		args->crsa_target_highest_slotid);

	fc_tbl = &cps->clp->cl_session->fc_slot_table;

	status = htonl(NFS4_OK);

	nfs41_set_target_slotid(fc_tbl, args->crsa_target_highest_slotid);
	nfs41_server_notify_target_slotid_update(cps->clp);
out:
	dprintk("%s: exit with status = %d\n", __func__, ntohl(status));
	return status;
}
#endif /* CONFIG_NFS_V4_1 */
