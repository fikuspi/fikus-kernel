/*
 * Access vector cache interface for object managers.
 *
 * Author : Stephen Smalley, <sds@epoch.ncsc.mil>
 */
#ifndef _SEFIKUS_AVC_H_
#define _SEFIKUS_AVC_H_

#include <fikus/stddef.h>
#include <fikus/errno.h>
#include <fikus/kernel.h>
#include <fikus/kdev_t.h>
#include <fikus/spinlock.h>
#include <fikus/init.h>
#include <fikus/audit.h>
#include <fikus/lsm_audit.h>
#include <fikus/in6.h>
#include "flask.h"
#include "av_permissions.h"
#include "security.h"

#ifdef CONFIG_SECURITY_SEFIKUS_DEVELOP
extern int sefikus_enforcing;
#else
#define sefikus_enforcing 1
#endif

/*
 * An entry in the AVC.
 */
struct avc_entry;

struct task_struct;
struct inode;
struct sock;
struct sk_buff;

/*
 * AVC statistics
 */
struct avc_cache_stats {
	unsigned int lookups;
	unsigned int misses;
	unsigned int allocations;
	unsigned int reclaims;
	unsigned int frees;
};

/*
 * We only need this data after we have decided to send an audit message.
 */
struct sefikus_audit_data {
	u32 ssid;
	u32 tsid;
	u16 tclass;
	u32 requested;
	u32 audited;
	u32 denied;
	int result;
};

/*
 * AVC operations
 */

void __init avc_init(void);

static inline u32 avc_audit_required(u32 requested,
			      struct av_decision *avd,
			      int result,
			      u32 auditdeny,
			      u32 *deniedp)
{
	u32 denied, audited;
	denied = requested & ~avd->allowed;
	if (unlikely(denied)) {
		audited = denied & avd->auditdeny;
		/*
		 * auditdeny is TRICKY!  Setting a bit in
		 * this field means that ANY denials should NOT be audited if
		 * the policy contains an explicit dontaudit rule for that
		 * permission.  Take notice that this is unrelated to the
		 * actual permissions that were denied.  As an example lets
		 * assume:
		 *
		 * denied == READ
		 * avd.auditdeny & ACCESS == 0 (not set means explicit rule)
		 * auditdeny & ACCESS == 1
		 *
		 * We will NOT audit the denial even though the denied
		 * permission was READ and the auditdeny checks were for
		 * ACCESS
		 */
		if (auditdeny && !(auditdeny & avd->auditdeny))
			audited = 0;
	} else if (result)
		audited = denied = requested;
	else
		audited = requested & avd->auditallow;
	*deniedp = denied;
	return audited;
}

int slow_avc_audit(u32 ssid, u32 tsid, u16 tclass,
		   u32 requested, u32 audited, u32 denied,
		   struct common_audit_data *a,
		   unsigned flags);

/**
 * avc_audit - Audit the granting or denial of permissions.
 * @ssid: source security identifier
 * @tsid: target security identifier
 * @tclass: target security class
 * @requested: requested permissions
 * @avd: access vector decisions
 * @result: result from avc_has_perm_noaudit
 * @a:  auxiliary audit data
 * @flags: VFS walk flags
 *
 * Audit the granting or denial of permissions in accordance
 * with the policy.  This function is typically called by
 * avc_has_perm() after a permission check, but can also be
 * called directly by callers who use avc_has_perm_noaudit()
 * in order to separate the permission check from the auditing.
 * For example, this separation is useful when the permission check must
 * be performed under a lock, to allow the lock to be released
 * before calling the auditing code.
 */
static inline int avc_audit(u32 ssid, u32 tsid,
			    u16 tclass, u32 requested,
			    struct av_decision *avd,
			    int result,
			    struct common_audit_data *a)
{
	u32 audited, denied;
	audited = avc_audit_required(requested, avd, result, 0, &denied);
	if (likely(!audited))
		return 0;
	return slow_avc_audit(ssid, tsid, tclass,
			      requested, audited, denied,
			      a, 0);
}

#define AVC_STRICT 1 /* Ignore permissive mode. */
int avc_has_perm_noaudit(u32 ssid, u32 tsid,
			 u16 tclass, u32 requested,
			 unsigned flags,
			 struct av_decision *avd);

int avc_has_perm(u32 ssid, u32 tsid,
		 u16 tclass, u32 requested,
		 struct common_audit_data *auditdata);

u32 avc_policy_seqno(void);

#define AVC_CALLBACK_GRANT		1
#define AVC_CALLBACK_TRY_REVOKE		2
#define AVC_CALLBACK_REVOKE		4
#define AVC_CALLBACK_RESET		8
#define AVC_CALLBACK_AUDITALLOW_ENABLE	16
#define AVC_CALLBACK_AUDITALLOW_DISABLE	32
#define AVC_CALLBACK_AUDITDENY_ENABLE	64
#define AVC_CALLBACK_AUDITDENY_DISABLE	128

int avc_add_callback(int (*callback)(u32 event), u32 events);

/* Exported to sefikusfs */
int avc_get_hash_stats(char *page);
extern unsigned int avc_cache_threshold;

/* Attempt to free avc node cache */
void avc_disable(void);

#ifdef CONFIG_SECURITY_SEFIKUS_AVC_STATS
DECLARE_PER_CPU(struct avc_cache_stats, avc_cache_stats);
#endif

#endif /* _SEFIKUS_AVC_H_ */

