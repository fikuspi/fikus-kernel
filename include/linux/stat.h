#ifndef _FIKUS_STAT_H
#define _FIKUS_STAT_H


#include <asm/stat.h>
#include <uapi/fikus/stat.h>

#define S_IRWXUGO	(S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)

#define UTIME_NOW	((1l << 30) - 1l)
#define UTIME_OMIT	((1l << 30) - 2l)

#include <fikus/types.h>
#include <fikus/time.h>
#include <fikus/uidgid.h>

struct kstat {
	u64		ino;
	dev_t		dev;
	umode_t		mode;
	unsigned int	nlink;
	kuid_t		uid;
	kgid_t		gid;
	dev_t		rdev;
	loff_t		size;
	struct timespec  atime;
	struct timespec	mtime;
	struct timespec	ctime;
	unsigned long	blksize;
	unsigned long long	blocks;
};

#endif
