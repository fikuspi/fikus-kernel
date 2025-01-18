/*
 * SEFikus support for the Audit LSM hooks
 *
 * Most of below header was moved from include/fikus/sefikus.h which
 * is released under below copyrights:
 *
 * Author: James Morris <jmorris@redhat.com>
 *
 * Copyright (C) 2005 Red Hat, Inc., James Morris <jmorris@redhat.com>
 * Copyright (C) 2006 Trusted Computer Solutions, Inc. <dgoeddel@trustedcs.com>
 * Copyright (C) 2006 IBM Corporation, Timothy R. Chavez <tinytim@us.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 */

#ifndef _SEFIKUS_AUDIT_H
#define _SEFIKUS_AUDIT_H

/**
 *	sefikus_audit_rule_init - alloc/init an sefikus audit rule structure.
 *	@field: the field this rule refers to
 *	@op: the operater the rule uses
 *	@rulestr: the text "target" of the rule
 *	@rule: pointer to the new rule structure returned via this
 *
 *	Returns 0 if successful, -errno if not.  On success, the rule structure
 *	will be allocated internally.  The caller must free this structure with
 *	sefikus_audit_rule_free() after use.
 */
int sefikus_audit_rule_init(u32 field, u32 op, char *rulestr, void **rule);

/**
 *	sefikus_audit_rule_free - free an sefikus audit rule structure.
 *	@rule: pointer to the audit rule to be freed
 *
 *	This will free all memory associated with the given rule.
 *	If @rule is NULL, no operation is performed.
 */
void sefikus_audit_rule_free(void *rule);

/**
 *	sefikus_audit_rule_match - determine if a context ID matches a rule.
 *	@sid: the context ID to check
 *	@field: the field this rule refers to
 *	@op: the operater the rule uses
 *	@rule: pointer to the audit rule to check against
 *	@actx: the audit context (can be NULL) associated with the check
 *
 *	Returns 1 if the context id matches the rule, 0 if it does not, and
 *	-errno on failure.
 */
int sefikus_audit_rule_match(u32 sid, u32 field, u32 op, void *rule,
			     struct audit_context *actx);

/**
 *	sefikus_audit_rule_known - check to see if rule contains sefikus fields.
 *	@rule: rule to be checked
 *	Returns 1 if there are sefikus fields specified in the rule, 0 otherwise.
 */
int sefikus_audit_rule_known(struct audit_krule *krule);

#endif /* _SEFIKUS_AUDIT_H */

