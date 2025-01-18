/*
 * xfrm_proc.c
 *
 * Copyright (C)2006-2007 USAGI/WIDE Project
 *
 * Authors:	Masahide NAKAMURA <nakam@fikus-ipv6.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <fikus/proc_fs.h>
#include <fikus/seq_file.h>
#include <fikus/export.h>
#include <net/snmp.h>
#include <net/xfrm.h>

static const struct snmp_mib xfrm_mib_list[] = {
	SNMP_MIB_ITEM("XfrmInError", FIKUS_MIB_XFRMINERROR),
	SNMP_MIB_ITEM("XfrmInBufferError", FIKUS_MIB_XFRMINBUFFERERROR),
	SNMP_MIB_ITEM("XfrmInHdrError", FIKUS_MIB_XFRMINHDRERROR),
	SNMP_MIB_ITEM("XfrmInNoStates", FIKUS_MIB_XFRMINNOSTATES),
	SNMP_MIB_ITEM("XfrmInStateProtoError", FIKUS_MIB_XFRMINSTATEPROTOERROR),
	SNMP_MIB_ITEM("XfrmInStateModeError", FIKUS_MIB_XFRMINSTATEMODEERROR),
	SNMP_MIB_ITEM("XfrmInStateSeqError", FIKUS_MIB_XFRMINSTATESEQERROR),
	SNMP_MIB_ITEM("XfrmInStateExpired", FIKUS_MIB_XFRMINSTATEEXPIRED),
	SNMP_MIB_ITEM("XfrmInStateMismatch", FIKUS_MIB_XFRMINSTATEMISMATCH),
	SNMP_MIB_ITEM("XfrmInStateInvalid", FIKUS_MIB_XFRMINSTATEINVALID),
	SNMP_MIB_ITEM("XfrmInTmplMismatch", FIKUS_MIB_XFRMINTMPLMISMATCH),
	SNMP_MIB_ITEM("XfrmInNoPols", FIKUS_MIB_XFRMINNOPOLS),
	SNMP_MIB_ITEM("XfrmInPolBlock", FIKUS_MIB_XFRMINPOLBLOCK),
	SNMP_MIB_ITEM("XfrmInPolError", FIKUS_MIB_XFRMINPOLERROR),
	SNMP_MIB_ITEM("XfrmOutError", FIKUS_MIB_XFRMOUTERROR),
	SNMP_MIB_ITEM("XfrmOutBundleGenError", FIKUS_MIB_XFRMOUTBUNDLEGENERROR),
	SNMP_MIB_ITEM("XfrmOutBundleCheckError", FIKUS_MIB_XFRMOUTBUNDLECHECKERROR),
	SNMP_MIB_ITEM("XfrmOutNoStates", FIKUS_MIB_XFRMOUTNOSTATES),
	SNMP_MIB_ITEM("XfrmOutStateProtoError", FIKUS_MIB_XFRMOUTSTATEPROTOERROR),
	SNMP_MIB_ITEM("XfrmOutStateModeError", FIKUS_MIB_XFRMOUTSTATEMODEERROR),
	SNMP_MIB_ITEM("XfrmOutStateSeqError", FIKUS_MIB_XFRMOUTSTATESEQERROR),
	SNMP_MIB_ITEM("XfrmOutStateExpired", FIKUS_MIB_XFRMOUTSTATEEXPIRED),
	SNMP_MIB_ITEM("XfrmOutPolBlock", FIKUS_MIB_XFRMOUTPOLBLOCK),
	SNMP_MIB_ITEM("XfrmOutPolDead", FIKUS_MIB_XFRMOUTPOLDEAD),
	SNMP_MIB_ITEM("XfrmOutPolError", FIKUS_MIB_XFRMOUTPOLERROR),
	SNMP_MIB_ITEM("XfrmFwdHdrError", FIKUS_MIB_XFRMFWDHDRERROR),
	SNMP_MIB_ITEM("XfrmOutStateInvalid", FIKUS_MIB_XFRMOUTSTATEINVALID),
	SNMP_MIB_ITEM("XfrmAcquireError", FIKUS_MIB_XFRMACQUIREERROR),
	SNMP_MIB_SENTINEL
};

static int xfrm_statistics_seq_show(struct seq_file *seq, void *v)
{
	struct net *net = seq->private;
	int i;
	for (i=0; xfrm_mib_list[i].name; i++)
		seq_printf(seq, "%-24s\t%lu\n", xfrm_mib_list[i].name,
			   snmp_fold_field((void __percpu **)
					   net->mib.xfrm_statistics,
					   xfrm_mib_list[i].entry));
	return 0;
}

static int xfrm_statistics_seq_open(struct inode *inode, struct file *file)
{
	return single_open_net(inode, file, xfrm_statistics_seq_show);
}

static const struct file_operations xfrm_statistics_seq_fops = {
	.owner	 = THIS_MODULE,
	.open	 = xfrm_statistics_seq_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release_net,
};

int __net_init xfrm_proc_init(struct net *net)
{
	if (!proc_create("xfrm_stat", S_IRUGO, net->proc_net,
			 &xfrm_statistics_seq_fops))
		return -ENOMEM;
	return 0;
}

void xfrm_proc_fini(struct net *net)
{
	remove_proc_entry("xfrm_stat", net->proc_net);
}
