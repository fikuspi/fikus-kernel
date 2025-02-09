#include <asm/div64.h>
#include <fikus/reciprocal_div.h>
#include <fikus/export.h>

u32 reciprocal_value(u32 k)
{
	u64 val = (1LL << 32) + (k - 1);
	do_div(val, k);
	return (u32)val;
}
EXPORT_SYMBOL(reciprocal_value);
