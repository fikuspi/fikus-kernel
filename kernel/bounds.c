/*
 * Generate definitions needed by the preprocessor.
 * This code generates raw asm output which is post-processed
 * to extract and format the required data.
 */

#define __GENERATING_BOUNDS_H
/* Include headers that define the enum constants of interest */
#include <fikus/page-flags.h>
#include <fikus/mmzone.h>
#include <fikus/kbuild.h>
#include <fikus/page_cgroup.h>

void foo(void)
{
	/* The enum constants to put into include/generated/bounds.h */
	DEFINE(NR_PAGEFLAGS, __NR_PAGEFLAGS);
	DEFINE(MAX_NR_ZONES, __MAX_NR_ZONES);
	DEFINE(NR_PCG_FLAGS, __NR_PCG_FLAGS);
	/* End of constants */
}
