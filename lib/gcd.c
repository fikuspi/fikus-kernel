#include <fikus/kernel.h>
#include <fikus/gcd.h>
#include <fikus/export.h>

/* Greatest common divisor */
unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r;

	if (a < b)
		swap(a, b);

	if (!b)
		return a;
	while ((r = a % b) != 0) {
		a = b;
		b = r;
	}
	return b;
}
EXPORT_SYMBOL_GPL(gcd);
