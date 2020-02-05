// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright 2019 Google LLC.
 */

#include <linux/filter.h>
#include <linux/bpf.h>
#include <linux/btf.h>

const struct bpf_prog_ops lsm_prog_ops = {
};

static const struct bpf_func_proto *get_bpf_func_proto(
	enum bpf_func_id func_id, const struct bpf_prog *prog)
{
	switch (func_id) {
	case BPF_FUNC_map_lookup_elem:
		return &bpf_map_lookup_elem_proto;
	case BPF_FUNC_get_current_pid_tgid:
		return &bpf_get_current_pid_tgid_proto;
	case BPF_FUNC_sleep_enable:
		return &bpf_sleep_enable_proto;
	case BPF_FUNC_sleep_disable:
		return &bpf_sleep_disable_proto;
	default:
		return NULL;
	}
}

const struct bpf_verifier_ops lsm_verifier_ops = {
	.get_func_proto = get_bpf_func_proto,
	.is_valid_access = btf_ctx_access,
};
