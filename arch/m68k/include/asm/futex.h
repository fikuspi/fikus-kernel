#ifndef _ASM_M68K_FUTEX_H
#define _ASM_M68K_FUTEX_H

#ifdef __KERNEL__
#if !defined(CONFIG_MMU)
#include <asm-generic/futex.h>
#else	/* CONFIG_MMU */

#include <fikus/futex.h>
#include <fikus/uaccess.h>
#include <asm/errno.h>

static inline int
futex_atomic_cmpxchg_inatomic(u32 *uval, u32 __user *uaddr,
			      u32 oldval, u32 newval)
{
	u32 val;

	if (unlikely(get_user(val, uaddr) != 0))
		return -EFAULT;

	if (val == oldval && unlikely(put_user(newval, uaddr) != 0))
		return -EFAULT;

	*uval = val;

	return 0;
}

static inline int
futex_atomic_op_inuser(int encoded_op, u32 __user *uaddr)
{
	int op = (encoded_op >> 28) & 7;
	int cmp = (encoded_op >> 24) & 15;
	int oparg = (encoded_op << 8) >> 20;
	int cmparg = (encoded_op << 20) >> 20;
	int oldval, ret;
	u32 tmp;

	if (encoded_op & (FUTEX_OP_OPARG_SHIFT << 28))
		oparg = 1 << oparg;

	pagefault_disable();	/* implies preempt_disable() */

	ret = -EFAULT;
	if (unlikely(get_user(oldval, uaddr) != 0))
		goto out_pagefault_enable;

	ret = 0;
	tmp = oldval;

	switch (op) {
	case FUTEX_OP_SET:
		tmp = oparg;
		break;
	case FUTEX_OP_ADD:
		tmp += oparg;
		break;
	case FUTEX_OP_OR:
		tmp |= oparg;
		break;
	case FUTEX_OP_ANDN:
		tmp &= ~oparg;
		break;
	case FUTEX_OP_XOR:
		tmp ^= oparg;
		break;
	default:
		ret = -ENOSYS;
	}

	if (ret == 0 && unlikely(put_user(tmp, uaddr) != 0))
		ret = -EFAULT;

out_pagefault_enable:
	pagefault_enable();	/* subsumes preempt_enable() */

	if (ret == 0) {
		switch (cmp) {
		case FUTEX_OP_CMP_EQ: ret = (oldval == cmparg); break;
		case FUTEX_OP_CMP_NE: ret = (oldval != cmparg); break;
		case FUTEX_OP_CMP_LT: ret = (oldval < cmparg); break;
		case FUTEX_OP_CMP_GE: ret = (oldval >= cmparg); break;
		case FUTEX_OP_CMP_LE: ret = (oldval <= cmparg); break;
		case FUTEX_OP_CMP_GT: ret = (oldval > cmparg); break;
		default: ret = -ENOSYS;
		}
	}
	return ret;
}

#endif /* CONFIG_MMU */
#endif /* __KERNEL__ */
#endif /* _ASM_M68K_FUTEX_H */
