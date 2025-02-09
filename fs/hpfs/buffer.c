/*
 *  fikus/fs/hpfs/buffer.c
 *
 *  Mikulas Patocka (mikulas@artax.karlin.mff.cuni.cz), 1998-1999
 *
 *  general buffer i/o
 */
#include <fikus/sched.h>
#include <fikus/slab.h>
#include <fikus/blkdev.h>
#include "hpfs_fn.h"

void hpfs_prefetch_sectors(struct super_block *s, unsigned secno, int n)
{
	struct buffer_head *bh;
	struct blk_plug plug;

	if (n <= 0 || unlikely(secno >= hpfs_sb(s)->sb_fs_size))
		return;

	bh = sb_find_get_block(s, secno);
	if (bh) {
		if (buffer_uptodate(bh)) {
			brelse(bh);
			return;
		}
		brelse(bh);
	};

	blk_start_plug(&plug);
	while (n > 0) {
		if (unlikely(secno >= hpfs_sb(s)->sb_fs_size))
			break;
		sb_breadahead(s, secno);
		secno++;
		n--;
	}
	blk_finish_plug(&plug);
}

/* Map a sector into a buffer and return pointers to it and to the buffer. */

void *hpfs_map_sector(struct super_block *s, unsigned secno, struct buffer_head **bhp,
		 int ahead)
{
	struct buffer_head *bh;

	hpfs_lock_assert(s);

	hpfs_prefetch_sectors(s, secno, ahead);

	cond_resched();

	*bhp = bh = sb_bread(s, secno);
	if (bh != NULL)
		return bh->b_data;
	else {
		printk("HPFS: hpfs_map_sector: read error\n");
		return NULL;
	}
}

/* Like hpfs_map_sector but don't read anything */

void *hpfs_get_sector(struct super_block *s, unsigned secno, struct buffer_head **bhp)
{
	struct buffer_head *bh;
	/*return hpfs_map_sector(s, secno, bhp, 0);*/

	hpfs_lock_assert(s);

	cond_resched();

	if ((*bhp = bh = sb_getblk(s, secno)) != NULL) {
		if (!buffer_uptodate(bh)) wait_on_buffer(bh);
		set_buffer_uptodate(bh);
		return bh->b_data;
	} else {
		printk("HPFS: hpfs_get_sector: getblk failed\n");
		return NULL;
	}
}

/* Map 4 sectors into a 4buffer and return pointers to it and to the buffer. */

void *hpfs_map_4sectors(struct super_block *s, unsigned secno, struct quad_buffer_head *qbh,
		   int ahead)
{
	struct buffer_head *bh;
	char *data;

	hpfs_lock_assert(s);

	cond_resched();

	if (secno & 3) {
		printk("HPFS: hpfs_map_4sectors: unaligned read\n");
		return NULL;
	}

	hpfs_prefetch_sectors(s, secno, 4 + ahead);

	qbh->data = data = kmalloc(2048, GFP_NOFS);
	if (!data) {
		printk("HPFS: hpfs_map_4sectors: out of memory\n");
		goto bail;
	}

	qbh->bh[0] = bh = sb_bread(s, secno);
	if (!bh)
		goto bail0;
	memcpy(data, bh->b_data, 512);

	qbh->bh[1] = bh = sb_bread(s, secno + 1);
	if (!bh)
		goto bail1;
	memcpy(data + 512, bh->b_data, 512);

	qbh->bh[2] = bh = sb_bread(s, secno + 2);
	if (!bh)
		goto bail2;
	memcpy(data + 2 * 512, bh->b_data, 512);

	qbh->bh[3] = bh = sb_bread(s, secno + 3);
	if (!bh)
		goto bail3;
	memcpy(data + 3 * 512, bh->b_data, 512);

	return data;

 bail3:
	brelse(qbh->bh[2]);
 bail2:
	brelse(qbh->bh[1]);
 bail1:
	brelse(qbh->bh[0]);
 bail0:
	kfree(data);
	printk("HPFS: hpfs_map_4sectors: read error\n");
 bail:
	return NULL;
}

/* Don't read sectors */

void *hpfs_get_4sectors(struct super_block *s, unsigned secno,
                          struct quad_buffer_head *qbh)
{
	cond_resched();

	hpfs_lock_assert(s);

	if (secno & 3) {
		printk("HPFS: hpfs_get_4sectors: unaligned read\n");
		return NULL;
	}

	/*return hpfs_map_4sectors(s, secno, qbh, 0);*/
	if (!(qbh->data = kmalloc(2048, GFP_NOFS))) {
		printk("HPFS: hpfs_get_4sectors: out of memory\n");
		return NULL;
	}
	if (!(hpfs_get_sector(s, secno, &qbh->bh[0]))) goto bail0;
	if (!(hpfs_get_sector(s, secno + 1, &qbh->bh[1]))) goto bail1;
	if (!(hpfs_get_sector(s, secno + 2, &qbh->bh[2]))) goto bail2;
	if (!(hpfs_get_sector(s, secno + 3, &qbh->bh[3]))) goto bail3;
	memcpy(qbh->data, qbh->bh[0]->b_data, 512);
	memcpy(qbh->data + 512, qbh->bh[1]->b_data, 512);
	memcpy(qbh->data + 2*512, qbh->bh[2]->b_data, 512);
	memcpy(qbh->data + 3*512, qbh->bh[3]->b_data, 512);
	return qbh->data;

	bail3:	brelse(qbh->bh[2]);
	bail2:	brelse(qbh->bh[1]);
	bail1:	brelse(qbh->bh[0]);
	bail0:
	return NULL;
}
	

void hpfs_brelse4(struct quad_buffer_head *qbh)
{
	brelse(qbh->bh[3]);
	brelse(qbh->bh[2]);
	brelse(qbh->bh[1]);
	brelse(qbh->bh[0]);
	kfree(qbh->data);
}	

void hpfs_mark_4buffers_dirty(struct quad_buffer_head *qbh)
{
	memcpy(qbh->bh[0]->b_data, qbh->data, 512);
	memcpy(qbh->bh[1]->b_data, qbh->data + 512, 512);
	memcpy(qbh->bh[2]->b_data, qbh->data + 2 * 512, 512);
	memcpy(qbh->bh[3]->b_data, qbh->data + 3 * 512, 512);
	mark_buffer_dirty(qbh->bh[0]);
	mark_buffer_dirty(qbh->bh[1]);
	mark_buffer_dirty(qbh->bh[2]);
	mark_buffer_dirty(qbh->bh[3]);
}
