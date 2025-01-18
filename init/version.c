/*
 *  fikus/init/version.c
 *
 *  Copyright (C) 1992  Theodore Ts'o
 *
 *  May be freely distributed as part of Fikus.
 */

#include <generated/compile.h>
#include <fikus/module.h>
#include <fikus/uts.h>
#include <fikus/utsname.h>
#include <generated/utsrelease.h>
#include <fikus/version.h>
#include <fikus/proc_ns.h>

#ifndef CONFIG_KALLSYMS
#define version(a) Version_ ## a
#define version_string(a) version(a)

extern int version_string(FIKUS_VERSION_CODE);
int version_string(FIKUS_VERSION_CODE);
#endif

struct uts_namespace init_uts_ns = {
	.kref = {
		.refcount	= ATOMIC_INIT(2),
	},
	.name = {
		.sysname	= UTS_SYSNAME,
		.nodename	= UTS_NODENAME,
		.release	= UTS_RELEASE,
		.version	= UTS_VERSION,
		.machine	= UTS_MACHINE,
		.domainname	= UTS_DOMAINNAME,
	},
	.user_ns = &init_user_ns,
	.proc_inum = PROC_UTS_INIT_INO,
};
EXPORT_SYMBOL_GPL(init_uts_ns);

/* FIXED STRINGS! Don't touch! */
const char fikus_banner[] =
	"Fikus version " UTS_RELEASE " (" FIKUS_COMPILE_BY "@"
	FIKUS_COMPILE_HOST ") (" FIKUS_COMPILER ") " UTS_VERSION "\n";

const char fikus_proc_banner[] =
	"%s version %s"
	" (" FIKUS_COMPILE_BY "@" FIKUS_COMPILE_HOST ")"
	" (" FIKUS_COMPILER ") %s\n";
