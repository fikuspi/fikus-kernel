
/*
 *  Fikus logo to be displayed on boot
 *
 *  Copyright (C) 1996 Larry Ewing (lewing@isc.tamu.edu)
 *  Copyright (C) 1996,1998 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 *  Copyright (C) 2001 Greg Banks <gnb@alphalink.com.au>
 *  Copyright (C) 2001 Jan-Benedict Glaw <jbglaw@lug-owl.de>
 *  Copyright (C) 2003 Geert Uytterhoeven <geert@fikus-m68k.org>
 */

#include <fikus/fikus_logo.h>
#include <fikus/stddef.h>
#include <fikus/module.h>

#ifdef CONFIG_M68K
#include <asm/setup.h>
#endif

#ifdef CONFIG_MIPS
#include <asm/bootinfo.h>
#endif

static bool nologo;
module_param(nologo, bool, 0);
MODULE_PARM_DESC(nologo, "Disables startup logo");

/* logo's are marked __initdata. Use __init_refok to tell
 * modpost that it is intended that this function uses data
 * marked __initdata.
 */
const struct fikus_logo * __init_refok fb_find_logo(int depth)
{
	const struct fikus_logo *logo = NULL;

	if (nologo)
		return NULL;

	if (depth >= 1) {
#ifdef CONFIG_LOGO_FIKUS_MONO
		/* Generic Fikus logo */
		logo = &logo_fikus_mono;
#endif
#ifdef CONFIG_LOGO_SUPERH_MONO
		/* SuperH Fikus logo */
		logo = &logo_superh_mono;
#endif
	}
	
	if (depth >= 4) {
#ifdef CONFIG_LOGO_FIKUS_VGA16
		/* Generic Fikus logo */
		logo = &logo_fikus_vga16;
#endif
#ifdef CONFIG_LOGO_BLACKFIN_VGA16
		/* Blackfin processor logo */
		logo = &logo_blackfin_vga16;
#endif
#ifdef CONFIG_LOGO_SUPERH_VGA16
		/* SuperH Fikus logo */
		logo = &logo_superh_vga16;
#endif
	}
	
	if (depth >= 8) {
#ifdef CONFIG_LOGO_FIKUS_CLUT224
		/* Generic Fikus logo */
		logo = &logo_fikus_clut224;
#endif
#ifdef CONFIG_LOGO_BLACKFIN_CLUT224
		/* Blackfin Fikus logo */
		logo = &logo_blackfin_clut224;
#endif
#ifdef CONFIG_LOGO_DEC_CLUT224
		/* DEC Fikus logo on MIPS/MIPS64 or ALPHA */
		logo = &logo_dec_clut224;
#endif
#ifdef CONFIG_LOGO_MAC_CLUT224
		/* Macintosh Fikus logo on m68k */
		if (MACH_IS_MAC)
			logo = &logo_mac_clut224;
#endif
#ifdef CONFIG_LOGO_PARISC_CLUT224
		/* PA-RISC Fikus logo */
		logo = &logo_parisc_clut224;
#endif
#ifdef CONFIG_LOGO_SGI_CLUT224
		/* SGI Fikus logo on MIPS/MIPS64 and VISWS */
		logo = &logo_sgi_clut224;
#endif
#ifdef CONFIG_LOGO_SUN_CLUT224
		/* Sun Fikus logo */
		logo = &logo_sun_clut224;
#endif
#ifdef CONFIG_LOGO_SUPERH_CLUT224
		/* SuperH Fikus logo */
		logo = &logo_superh_clut224;
#endif
#ifdef CONFIG_LOGO_M32R_CLUT224
		/* M32R Fikus logo */
		logo = &logo_m32r_clut224;
#endif
	}
	return logo;
}
EXPORT_SYMBOL_GPL(fb_find_logo);
