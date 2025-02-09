/*
 * btnode.h - NILFS B-tree node cache
 *
 * Copyright (C) 2005-2008 Nippon Telegraph and Telephone Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Written by Seiji Kihara <kihara@osrg.net>
 * Revised by Ryusuke Konishi <ryusuke@osrg.net>
 */

#ifndef _NILFS_BTNODE_H
#define _NILFS_BTNODE_H

#include <fikus/types.h>
#include <fikus/buffer_head.h>
#include <fikus/fs.h>
#include <fikus/backing-dev.h>

/**
 * struct nilfs_btnode_chkey_ctxt - change key context
 * @oldkey: old key of block's moving content
 * @newkey: new key for block's content
 * @bh: buffer head of old buffer
 * @newbh: buffer head of new buffer
 */
struct nilfs_btnode_chkey_ctxt {
	__u64 oldkey;
	__u64 newkey;
	struct buffer_head *bh;
	struct buffer_head *newbh;
};

void nilfs_btnode_cache_clear(struct address_space *);
struct buffer_head *nilfs_btnode_create_block(struct address_space *btnc,
					      __u64 blocknr);
int nilfs_btnode_submit_block(struct address_space *, __u64, sector_t, int,
			      struct buffer_head **, sector_t *);
void nilfs_btnode_delete(struct buffer_head *);
int nilfs_btnode_prepare_change_key(struct address_space *,
				    struct nilfs_btnode_chkey_ctxt *);
void nilfs_btnode_commit_change_key(struct address_space *,
				    struct nilfs_btnode_chkey_ctxt *);
void nilfs_btnode_abort_change_key(struct address_space *,
				   struct nilfs_btnode_chkey_ctxt *);

#endif	/* _NILFS_BTNODE_H */
