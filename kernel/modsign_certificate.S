#include <fikus/export.h>

#define GLOBAL(name)	\
	.globl VMFIKUS_SYMBOL(name);	\
	VMFIKUS_SYMBOL(name):

	.section ".init.data","aw"

GLOBAL(modsign_certificate_list)
	.incbin "signing_key.x509"
	.incbin "extra_certificates"
GLOBAL(modsign_certificate_list_end)
