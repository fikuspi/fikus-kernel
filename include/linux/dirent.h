#ifndef _FIKUS_DIRENT_H
#define _FIKUS_DIRENT_H

struct fikus_dirent64 {
	u64		d_ino;
	s64		d_off;
	unsigned short	d_reclen;
	unsigned char	d_type;
	char		d_name[0];
};

#endif
