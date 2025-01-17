/*
 * Linux Security Module interfaces
 *
 * Copyright (C) 2001 WireX Communications, Inc <chris@wirex.com>
 * Copyright (C) 2001 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2001 Networks Associates Technology, Inc <ssmalley@nai.com>
 * Copyright (C) 2001 James Morris <jmorris@intercode.com.au>
 * Copyright (C) 2001 Silicon Graphics, Inc. (Trust Technology Group)
 * Copyright (C) 2015 Intel Corporation.
 * Copyright (C) 2015 Casey Schaufler <casey@schaufler-ca.com>
 * Copyright (C) 2016 Mellanox Techonologies
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Due to this file being licensed under the GPL there is controversy over
 *	whether this permits you to write a module that #includes this file
 *	without placing your module under the GPL.  Please consult a lawyer for
 *	advice before doing this.
 *
 */

#ifndef __LINUX_LSM_HOOKS_H
#define __LINUX_LSM_HOOKS_H

#include <linux/security.h>
#include <linux/init.h>
#include <linux/rculist.h>
#include <linux/static_call.h>
#include <linux/unroll.h>
#include <linux/jump_label.h>
#include <linux/lsm_count.h>

#define SECURITY_HOOK_ACTIVE_KEY(HOOK, IDX) security_hook_active_##HOOK##_##IDX

/*
 * Identifier for the LSM static calls.
 * HOOK is an LSM hook as defined in linux/lsm_hookdefs.h
 * IDX is the index of the static call. 0 <= NUM < MAX_LSM_COUNT
 */
#define LSM_STATIC_CALL(HOOK, IDX) lsm_static_call_##HOOK##_##IDX

/*
 * Call the macro M for each LSM hook MAX_LSM_COUNT times.
 */
#define LSM_LOOP_UNROLL(M, ...) 		\
do {						\
	UNROLL(MAX_LSM_COUNT, M, __VA_ARGS__)	\
} while (0)

#define LSM_DEFINE_UNROLL(M, ...) UNROLL(MAX_LSM_COUNT, M, __VA_ARGS__)

union security_list_options {
	#define LSM_HOOK(RET, DEFAULT, NAME, ...) RET (*NAME)(__VA_ARGS__);
	#include "lsm_hook_defs.h"
	#undef LSM_HOOK
	void *lsm_callback;
};

/*
 * @key: static call key as defined by STATIC_CALL_KEY
 * @trampoline: static call trampoline as defined by STATIC_CALL_TRAMP
 * @hl: The security_hook_list as initialized by the owning LSM.
 * @active: Enabled when the static call has an LSM hook associated.
 */
struct lsm_static_call {
	struct static_call_key *key;
	void *trampoline;
	struct security_hook_list *hl;
	/* this needs to be true or false based on what the key defaults to */
	struct static_key_false *active;
};

/*
 * Table of the static calls for each LSM hook.
 * Once the LSMs are initialized, their callbacks will be copied to these
 * tables such that the calls are filled backwards (from last to first).
 * This way, we can jump directly to the first used static call, and execute
 * all of them after. This essentially makes the entry point
 * dynamic to adapt the number of static calls to the number of callbacks.
 */
struct lsm_static_calls_table {
	#define LSM_HOOK(RET, DEFAULT, NAME, ...) \
		struct lsm_static_call NAME[MAX_LSM_COUNT];
	#include <linux/lsm_hook_defs.h>
	#undef LSM_HOOK
} __randomize_layout;

/*
 * Security module hook list structure.
 * For use with generic list macros for common operations.
 *
 * struct security_hook_list - Contents of a cacheable, mappable object.
 * @scalls: The beginning of the array of static calls assigned to this hook.
 * @hook: The callback for the hook.
 * @lsm: The name of the lsm that owns this hook.
 * @default_state: The state of the LSM hook when initialized. If set to false,
 * the static key guarding the hook will be set to disabled.
 */
struct security_hook_list {
	struct lsm_static_call	*scalls;
	union security_list_options	hook;
	const char			*lsm;
	bool				default_state;
} __randomize_layout;

/*
 * Security blob size or offset data.
 */
struct lsm_blob_sizes {
	int	lbs_cred;
	int	lbs_file;
	int	lbs_inode;
	int	lbs_superblock;
	int	lbs_ipc;
	int	lbs_msg_msg;
	int	lbs_task;
};

/*
 * LSM_RET_VOID is used as the default value in LSM_HOOK definitions for void
 * LSM hooks (in include/linux/lsm_hook_defs.h).
 */
#define LSM_RET_VOID ((void) 0)

/*
 * Initializing a security_hook_list structure takes
 * up a lot of space in a source file. This macro takes
 * care of the common case and reduces the amount of
 * text involved.
 */
#define LSM_HOOK_INIT(NAME, CALLBACK)			\
	{						\
		.scalls = static_calls_table.NAME,	\
		.hook = { .NAME = CALLBACK },		\
		.default_state = true			\
	}

#define LSM_HOOK_INIT_DISABLED(NAME, CALLBACK)		\
	{						\
		.scalls = static_calls_table.NAME,	\
		.hook = { .NAME = CALLBACK },		\
		.default_state = false			\
	}

extern char *lsm_names;

extern void security_add_hooks(struct security_hook_list *hooks, int count,
				const char *lsm);

#define LSM_FLAG_LEGACY_MAJOR	BIT(0)
#define LSM_FLAG_EXCLUSIVE	BIT(1)

enum lsm_order {
	LSM_ORDER_FIRST = -1,	/* This is only for capabilities. */
	LSM_ORDER_MUTABLE = 0,
	LSM_ORDER_LAST = 1,	/* This is only for integrity. */
};

struct lsm_info {
	const char *name;	/* Required. */
	enum lsm_order order;	/* Optional: default is LSM_ORDER_MUTABLE */
	unsigned long flags;	/* Optional: flags describing LSM */
	int *enabled;		/* Optional: controlled by CONFIG_LSM */
	int (*init)(void);	/* Required. */
	struct lsm_blob_sizes *blobs; /* Optional: for blob sharing. */
};

extern struct lsm_info __start_lsm_info[], __end_lsm_info[];
extern struct lsm_info __start_early_lsm_info[], __end_early_lsm_info[];

#define DEFINE_LSM(lsm)							\
	static struct lsm_info __lsm_##lsm				\
		__used __section(".lsm_info.init")			\
		__aligned(sizeof(unsigned long))

#define DEFINE_EARLY_LSM(lsm)						\
	static struct lsm_info __early_lsm_##lsm			\
		__used __section(".early_lsm_info.init")		\
		__aligned(sizeof(unsigned long))

extern int lsm_inode_alloc(struct inode *inode);
extern struct lsm_static_calls_table static_calls_table __ro_after_init;

#endif /* ! __LINUX_LSM_HOOKS_H */
