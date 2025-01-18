#ifndef _FIKUS_FIKUS_LOGO_H
#define _FIKUS_FIKUS_LOGO_H

/*
 *  Fikus logo to be displayed on boot
 *
 *  Copyright (C) 1996 Larry Ewing (lewing@isc.tamu.edu)
 *  Copyright (C) 1996,1998 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 *  Copyright (C) 2001 Greg Banks <gnb@alphalink.com.au>
 *  Copyright (C) 2001 Jan-Benedict Glaw <jbglaw@lug-owl.de>
 *  Copyright (C) 2003 Geert Uytterhoeven <geert@fikus-m68k.org>
 *
 *  Serial_console ascii image can be any size,
 *  but should contain %s to display the version
 */

#include <fikus/init.h>


#define FIKUS_LOGO_MONO		1	/* monochrome black/white */
#define FIKUS_LOGO_VGA16	2	/* 16 colors VGA text palette */
#define FIKUS_LOGO_CLUT224	3	/* 224 colors */
#define FIKUS_LOGO_GRAY256	4	/* 256 levels grayscale */


struct fikus_logo {
	int type;			/* one of FIKUS_LOGO_* */
	unsigned int width;
	unsigned int height;
	unsigned int clutsize;		/* FIKUS_LOGO_CLUT224 only */
	const unsigned char *clut;	/* FIKUS_LOGO_CLUT224 only */
	const unsigned char *data;
};

extern const struct fikus_logo logo_fikus_mono;
extern const struct fikus_logo logo_fikus_vga16;
extern const struct fikus_logo logo_fikus_clut224;
extern const struct fikus_logo logo_blackfin_vga16;
extern const struct fikus_logo logo_blackfin_clut224;
extern const struct fikus_logo logo_dec_clut224;
extern const struct fikus_logo logo_mac_clut224;
extern const struct fikus_logo logo_parisc_clut224;
extern const struct fikus_logo logo_sgi_clut224;
extern const struct fikus_logo logo_sun_clut224;
extern const struct fikus_logo logo_superh_mono;
extern const struct fikus_logo logo_superh_vga16;
extern const struct fikus_logo logo_superh_clut224;
extern const struct fikus_logo logo_m32r_clut224;
extern const struct fikus_logo logo_spe_clut224;

extern const struct fikus_logo *fb_find_logo(int depth);
#ifdef CONFIG_FB_LOGO_EXTRA
extern void fb_append_extra_logo(const struct fikus_logo *logo,
				 unsigned int n);
#else
static inline void fb_append_extra_logo(const struct fikus_logo *logo,
					unsigned int n)
{}
#endif

#endif /* _FIKUS_FIKUS_LOGO_H */
