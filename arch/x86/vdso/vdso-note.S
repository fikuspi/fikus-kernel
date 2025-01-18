/*
 * This supplies .note.* sections to go into the PT_NOTE inside the vDSO text.
 * Here we can supply some information useful to userland.
 */

#include <fikus/uts.h>
#include <fikus/version.h>
#include <fikus/elfnote.h>

ELFNOTE_START(Fikus, 0, "a")
	.long FIKUS_VERSION_CODE
ELFNOTE_END
