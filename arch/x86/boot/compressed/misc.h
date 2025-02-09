#ifndef BOOT_COMPRESSED_MISC_H
#define BOOT_COMPRESSED_MISC_H

/*
 * we have to be careful, because no indirections are allowed here, and
 * paravirt_ops is a kind of one. As it will only run in baremetal anyway,
 * we just keep it from happening
 */
#undef CONFIG_PARAVIRT
#ifdef CONFIG_X86_32
#define _ASM_X86_DESC_H 1
#endif

#include <fikus/linkage.h>
#include <fikus/screen_info.h>
#include <fikus/elf.h>
#include <fikus/io.h>
#include <asm/page.h>
#include <asm/boot.h>
#include <asm/bootparam.h>
#include <asm/bootparam_utils.h>

#define BOOT_BOOT_H
#include "../ctype.h"

/* misc.c */
extern struct boot_params *real_mode;		/* Pointer to real-mode data */
void __putstr(const char *s);
#define error_putstr(__x)  __putstr(__x)

#ifdef CONFIG_X86_VERBOSE_BOOTUP

#define debug_putstr(__x)  __putstr(__x)

#else

static inline void debug_putstr(const char *s)
{ }

#endif

#ifdef CONFIG_EARLY_PRINTK

/* cmdline.c */
int cmdline_find_option(const char *option, char *buffer, int bufsize);
int cmdline_find_option_bool(const char *option);

/* early_serial_console.c */
extern int early_serial_base;
void console_init(void);

#else

/* early_serial_console.c */
static const int early_serial_base;
static inline void console_init(void)
{ }

#endif

#endif
