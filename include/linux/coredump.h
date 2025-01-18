#ifndef _FIKUS_COREDUMP_H
#define _FIKUS_COREDUMP_H

#include <fikus/types.h>
#include <fikus/mm.h>
#include <fikus/fs.h>
#include <asm/siginfo.h>

/*
 * These are the only things you should do on a core-file: use only these
 * functions to write out all the necessary info.
 */
extern int dump_write(struct file *file, const void *addr, int nr);
extern int dump_seek(struct file *file, loff_t off);
#ifdef CONFIG_COREDUMP
extern void do_coredump(siginfo_t *siginfo);
#else
static inline void do_coredump(siginfo_t *siginfo) {}
#endif

#endif /* _FIKUS_COREDUMP_H */
