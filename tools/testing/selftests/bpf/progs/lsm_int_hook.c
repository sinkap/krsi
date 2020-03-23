// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright 2020 Google LLC.
 */

#include <linux/bpf.h>
#include <stdbool.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include  <errno.h>
#include "lsm_helpers.h"

char _license[] SEC("license") = "GPL";

struct lsm_prog_result result = {
	.monitored_pid = 0,
	.count = 0,
};

/*
 * Define some of the structs used in the BPF program.
 * Only the field names and their sizes need to be the
 * same as the kernel type, the order is irrelevant.
 */
struct mm_struct {
	unsigned long start_brk, brk;
} __attribute__((preserve_access_index));

struct vm_area_struct {
	unsigned long vm_start, vm_end;
	struct mm_struct *vm_mm;
} __attribute__((preserve_access_index));

SEC("lsm/file_mprotect")
int BPF_PROG(test_int_hook, struct vm_area_struct *vma,
	     unsigned long reqprot, unsigned long prot, int ret)
{
	if (ret != 0)
		return ret;

	__u32 pid = bpf_get_current_pid_tgid();
	int is_heap = 0;

	is_heap = (vma->vm_start >= vma->vm_mm->start_brk &&
		   vma->vm_end <= vma->vm_mm->brk);

	if (is_heap && result.monitored_pid == pid) {
		result.count++;
		ret = -EPERM;
	}

	return ret;
}
