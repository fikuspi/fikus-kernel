/*
 * Copyright (c) 2013, Mellanox Technologies inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MLX5_QP_H
#define MLX5_QP_H

#include <fikus/mlx5/device.h>
#include <fikus/mlx5/driver.h>

#define MLX5_INVALID_LKEY	0x100

enum mlx5_qp_optpar {
	MLX5_QP_OPTPAR_ALT_ADDR_PATH		= 1 << 0,
	MLX5_QP_OPTPAR_RRE			= 1 << 1,
	MLX5_QP_OPTPAR_RAE			= 1 << 2,
	MLX5_QP_OPTPAR_RWE			= 1 << 3,
	MLX5_QP_OPTPAR_PKEY_INDEX		= 1 << 4,
	MLX5_QP_OPTPAR_Q_KEY			= 1 << 5,
	MLX5_QP_OPTPAR_RNR_TIMEOUT		= 1 << 6,
	MLX5_QP_OPTPAR_PRIMARY_ADDR_PATH	= 1 << 7,
	MLX5_QP_OPTPAR_SRA_MAX			= 1 << 8,
	MLX5_QP_OPTPAR_RRA_MAX			= 1 << 9,
	MLX5_QP_OPTPAR_PM_STATE			= 1 << 10,
	MLX5_QP_OPTPAR_RETRY_COUNT		= 1 << 12,
	MLX5_QP_OPTPAR_RNR_RETRY		= 1 << 13,
	MLX5_QP_OPTPAR_ACK_TIMEOUT		= 1 << 14,
	MLX5_QP_OPTPAR_PRI_PORT			= 1 << 16,
	MLX5_QP_OPTPAR_SRQN			= 1 << 18,
	MLX5_QP_OPTPAR_CQN_RCV			= 1 << 19,
	MLX5_QP_OPTPAR_DC_HS			= 1 << 20,
	MLX5_QP_OPTPAR_DC_KEY			= 1 << 21,
};

enum mlx5_qp_state {
	MLX5_QP_STATE_RST			= 0,
	MLX5_QP_STATE_INIT			= 1,
	MLX5_QP_STATE_RTR			= 2,
	MLX5_QP_STATE_RTS			= 3,
	MLX5_QP_STATE_SQER			= 4,
	MLX5_QP_STATE_SQD			= 5,
	MLX5_QP_STATE_ERR			= 6,
	MLX5_QP_STATE_SQ_DRAINING		= 7,
	MLX5_QP_STATE_SUSPENDED			= 9,
	MLX5_QP_NUM_STATE
};

enum {
	MLX5_QP_ST_RC				= 0x0,
	MLX5_QP_ST_UC				= 0x1,
	MLX5_QP_ST_UD				= 0x2,
	MLX5_QP_ST_XRC				= 0x3,
	MLX5_QP_ST_MLX				= 0x4,
	MLX5_QP_ST_DCI				= 0x5,
	MLX5_QP_ST_DCT				= 0x6,
	MLX5_QP_ST_QP0				= 0x7,
	MLX5_QP_ST_QP1				= 0x8,
	MLX5_QP_ST_RAW_ETHERTYPE		= 0x9,
	MLX5_QP_ST_RAW_IPV6			= 0xa,
	MLX5_QP_ST_SNIFFER			= 0xb,
	MLX5_QP_ST_SYNC_UMR			= 0xe,
	MLX5_QP_ST_PTP_1588			= 0xd,
	MLX5_QP_ST_REG_UMR			= 0xc,
	MLX5_QP_ST_MAX
};

enum {
	MLX5_QP_PM_MIGRATED			= 0x3,
	MLX5_QP_PM_ARMED			= 0x0,
	MLX5_QP_PM_REARM			= 0x1
};

enum {
	MLX5_NON_ZERO_RQ	= 0 << 24,
	MLX5_SRQ_RQ		= 1 << 24,
	MLX5_CRQ_RQ		= 2 << 24,
	MLX5_ZERO_LEN_RQ	= 3 << 24
};

enum {
	/* params1 */
	MLX5_QP_BIT_SRE				= 1 << 15,
	MLX5_QP_BIT_SWE				= 1 << 14,
	MLX5_QP_BIT_SAE				= 1 << 13,
	/* params2 */
	MLX5_QP_BIT_RRE				= 1 << 15,
	MLX5_QP_BIT_RWE				= 1 << 14,
	MLX5_QP_BIT_RAE				= 1 << 13,
	MLX5_QP_BIT_RIC				= 1 <<	4,
};

enum {
	MLX5_WQE_CTRL_CQ_UPDATE		= 2 << 2,
	MLX5_WQE_CTRL_SOLICITED		= 1 << 1,
};

enum {
	MLX5_SEND_WQE_BB	= 64,
};

enum {
	MLX5_WQE_FMR_PERM_LOCAL_READ	= 1 << 27,
	MLX5_WQE_FMR_PERM_LOCAL_WRITE	= 1 << 28,
	MLX5_WQE_FMR_PERM_REMOTE_READ	= 1 << 29,
	MLX5_WQE_FMR_PERM_REMOTE_WRITE	= 1 << 30,
	MLX5_WQE_FMR_PERM_ATOMIC	= 1 << 31
};

enum {
	MLX5_FENCE_MODE_NONE			= 0 << 5,
	MLX5_FENCE_MODE_INITIATOR_SMALL		= 1 << 5,
	MLX5_FENCE_MODE_STRONG_ORDERING		= 3 << 5,
	MLX5_FENCE_MODE_SMALL_AND_FENCE		= 4 << 5,
};

enum {
	MLX5_QP_LAT_SENSITIVE	= 1 << 28,
	MLX5_QP_ENABLE_SIG	= 1 << 31,
};

enum {
	MLX5_RCV_DBR	= 0,
	MLX5_SND_DBR	= 1,
};

struct mlx5_wqe_fmr_seg {
	__be32			flags;
	__be32			mem_key;
	__be64			buf_list;
	__be64			start_addr;
	__be64			reg_len;
	__be32			offset;
	__be32			page_size;
	u32			reserved[2];
};

struct mlx5_wqe_ctrl_seg {
	__be32			opmod_idx_opcode;
	__be32			qpn_ds;
	u8			signature;
	u8			rsvd[2];
	u8			fm_ce_se;
	__be32			imm;
};

struct mlx5_wqe_xrc_seg {
	__be32			xrc_srqn;
	u8			rsvd[12];
};

struct mlx5_wqe_masked_atomic_seg {
	__be64			swap_add;
	__be64			compare;
	__be64			swap_add_mask;
	__be64			compare_mask;
};

struct mlx5_av {
	union {
		struct {
			__be32	qkey;
			__be32	reserved;
		} qkey;
		__be64	dc_key;
	} key;
	__be32	dqp_dct;
	u8	stat_rate_sl;
	u8	fl_mlid;
	__be16	rlid;
	u8	reserved0[10];
	u8	tclass;
	u8	hop_limit;
	__be32	grh_gid_fl;
	u8	rgid[16];
};

struct mlx5_wqe_datagram_seg {
	struct mlx5_av	av;
};

struct mlx5_wqe_raddr_seg {
	__be64			raddr;
	__be32			rkey;
	u32			reserved;
};

struct mlx5_wqe_atomic_seg {
	__be64			swap_add;
	__be64			compare;
};

struct mlx5_wqe_data_seg {
	__be32			byte_count;
	__be32			lkey;
	__be64			addr;
};

struct mlx5_wqe_umr_ctrl_seg {
	u8		flags;
	u8		rsvd0[3];
	__be16		klm_octowords;
	__be16		bsf_octowords;
	__be64		mkey_mask;
	u8		rsvd1[32];
};

struct mlx5_seg_set_psv {
	__be32		psv_num;
	__be16		syndrome;
	__be16		status;
	__be32		transient_sig;
	__be32		ref_tag;
};

struct mlx5_seg_get_psv {
	u8		rsvd[19];
	u8		num_psv;
	__be32		l_key;
	__be64		va;
	__be32		psv_index[4];
};

struct mlx5_seg_check_psv {
	u8		rsvd0[2];
	__be16		err_coalescing_op;
	u8		rsvd1[2];
	__be16		xport_err_op;
	u8		rsvd2[2];
	__be16		xport_err_mask;
	u8		rsvd3[7];
	u8		num_psv;
	__be32		l_key;
	__be64		va;
	__be32		psv_index[4];
};

struct mlx5_rwqe_sig {
	u8	rsvd0[4];
	u8	signature;
	u8	rsvd1[11];
};

struct mlx5_wqe_signature_seg {
	u8	rsvd0[4];
	u8	signature;
	u8	rsvd1[11];
};

struct mlx5_wqe_inline_seg {
	__be32	byte_count;
};

struct mlx5_core_qp {
	void (*event)		(struct mlx5_core_qp *, int);
	int			qpn;
	atomic_t		refcount;
	struct completion	free;
	struct mlx5_rsc_debug	*dbg;
	int			pid;
};

struct mlx5_qp_path {
	u8			fl;
	u8			rsvd3;
	u8			free_ar;
	u8			pkey_index;
	u8			rsvd0;
	u8			grh_mlid;
	__be16			rlid;
	u8			ackto_lt;
	u8			mgid_index;
	u8			static_rate;
	u8			hop_limit;
	__be32			tclass_flowlabel;
	u8			rgid[16];
	u8			rsvd1[4];
	u8			sl;
	u8			port;
	u8			rsvd2[6];
};

struct mlx5_qp_context {
	__be32			flags;
	__be32			flags_pd;
	u8			mtu_msgmax;
	u8			rq_size_stride;
	__be16			sq_crq_size;
	__be32			qp_counter_set_usr_page;
	__be32			wire_qpn;
	__be32			log_pg_sz_remote_qpn;
	struct			mlx5_qp_path pri_path;
	struct			mlx5_qp_path alt_path;
	__be32			params1;
	u8			reserved2[4];
	__be32			next_send_psn;
	__be32			cqn_send;
	u8			reserved3[8];
	__be32			last_acked_psn;
	__be32			ssn;
	__be32			params2;
	__be32			rnr_nextrecvpsn;
	__be32			xrcd;
	__be32			cqn_recv;
	__be64			db_rec_addr;
	__be32			qkey;
	__be32			rq_type_srqn;
	__be32			rmsn;
	__be16			hw_sq_wqe_counter;
	__be16			sw_sq_wqe_counter;
	__be16			hw_rcyclic_byte_counter;
	__be16			hw_rq_counter;
	__be16			sw_rcyclic_byte_counter;
	__be16			sw_rq_counter;
	u8			rsvd0[5];
	u8			cgs;
	u8			cs_req;
	u8			cs_res;
	__be64			dc_access_key;
	u8			rsvd1[24];
};

struct mlx5_create_qp_mbox_in {
	struct mlx5_inbox_hdr	hdr;
	__be32			input_qpn;
	u8			rsvd0[4];
	__be32			opt_param_mask;
	u8			rsvd1[4];
	struct mlx5_qp_context	ctx;
	u8			rsvd3[16];
	__be64			pas[0];
};

struct mlx5_create_qp_mbox_out {
	struct mlx5_outbox_hdr	hdr;
	__be32			qpn;
	u8			rsvd0[4];
};

struct mlx5_destroy_qp_mbox_in {
	struct mlx5_inbox_hdr	hdr;
	__be32			qpn;
	u8			rsvd0[4];
};

struct mlx5_destroy_qp_mbox_out {
	struct mlx5_outbox_hdr	hdr;
	u8			rsvd0[8];
};

struct mlx5_modify_qp_mbox_in {
	struct mlx5_inbox_hdr	hdr;
	__be32			qpn;
	u8			rsvd1[4];
	__be32			optparam;
	u8			rsvd0[4];
	struct mlx5_qp_context	ctx;
};

struct mlx5_modify_qp_mbox_out {
	struct mlx5_outbox_hdr	hdr;
	u8			rsvd0[8];
};

struct mlx5_query_qp_mbox_in {
	struct mlx5_inbox_hdr	hdr;
	__be32			qpn;
	u8			rsvd[4];
};

struct mlx5_query_qp_mbox_out {
	struct mlx5_outbox_hdr	hdr;
	u8			rsvd1[8];
	__be32			optparam;
	u8			rsvd0[4];
	struct mlx5_qp_context	ctx;
	u8			rsvd2[16];
	__be64			pas[0];
};

struct mlx5_conf_sqp_mbox_in {
	struct mlx5_inbox_hdr	hdr;
	__be32			qpn;
	u8			rsvd[3];
	u8			type;
};

struct mlx5_conf_sqp_mbox_out {
	struct mlx5_outbox_hdr	hdr;
	u8			rsvd[8];
};

struct mlx5_alloc_xrcd_mbox_in {
	struct mlx5_inbox_hdr	hdr;
	u8			rsvd[8];
};

struct mlx5_alloc_xrcd_mbox_out {
	struct mlx5_outbox_hdr	hdr;
	__be32			xrcdn;
	u8			rsvd[4];
};

struct mlx5_dealloc_xrcd_mbox_in {
	struct mlx5_inbox_hdr	hdr;
	__be32			xrcdn;
	u8			rsvd[4];
};

struct mlx5_dealloc_xrcd_mbox_out {
	struct mlx5_outbox_hdr	hdr;
	u8			rsvd[8];
};

static inline struct mlx5_core_qp *__mlx5_qp_lookup(struct mlx5_core_dev *dev, u32 qpn)
{
	return radix_tree_lookup(&dev->priv.qp_table.tree, qpn);
}

int mlx5_core_create_qp(struct mlx5_core_dev *dev,
			struct mlx5_core_qp *qp,
			struct mlx5_create_qp_mbox_in *in,
			int inlen);
int mlx5_core_qp_modify(struct mlx5_core_dev *dev, enum mlx5_qp_state cur_state,
			enum mlx5_qp_state new_state,
			struct mlx5_modify_qp_mbox_in *in, int sqd_event,
			struct mlx5_core_qp *qp);
int mlx5_core_destroy_qp(struct mlx5_core_dev *dev,
			 struct mlx5_core_qp *qp);
int mlx5_core_qp_query(struct mlx5_core_dev *dev, struct mlx5_core_qp *qp,
		       struct mlx5_query_qp_mbox_out *out, int outlen);

int mlx5_core_xrcd_alloc(struct mlx5_core_dev *dev, u32 *xrcdn);
int mlx5_core_xrcd_dealloc(struct mlx5_core_dev *dev, u32 xrcdn);
void mlx5_init_qp_table(struct mlx5_core_dev *dev);
void mlx5_cleanup_qp_table(struct mlx5_core_dev *dev);
int mlx5_debug_qp_add(struct mlx5_core_dev *dev, struct mlx5_core_qp *qp);
void mlx5_debug_qp_remove(struct mlx5_core_dev *dev, struct mlx5_core_qp *qp);

#endif /* MLX5_QP_H */
