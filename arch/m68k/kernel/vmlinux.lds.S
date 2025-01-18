#if defined(CONFIG_MMU) && !defined(CONFIG_COLDFIRE)
PHDRS
{
  text PT_LOAD FILEHDR PHDRS FLAGS (7);
  data PT_LOAD FLAGS (7);
}
#ifdef CONFIG_SUN3
#include "vmfikus-sun3.lds"
#else
#include "vmfikus-std.lds"
#endif
#else
#include "vmfikus-nommu.lds"
#endif
