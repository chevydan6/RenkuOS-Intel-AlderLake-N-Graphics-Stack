/* SPDX-License-Identifier: MIT */
#ifndef RENKU_PORT_CONTEXT_CASES_H
#define RENKU_PORT_CONTEXT_CASES_H
#include <linux/printk.h>
#include <linux/kdev_t.h>
static void PortContextCases(struct completion_report *r)
{
	CHECK(r, current->comm[0] != 0 && current->comm[TASK_COMM_LEN - 1] == 0);
#ifdef RENKU_NATIVE
	thread_info info;
	CHECK(r, get_thread_info(find_thread(NULL), &info) == B_OK);
	CHECK(r, strncmp(current->comm, info.name, TASK_COMM_LEN - 1) == 0);
#else
	char name[TASK_COMM_LEN] = {0};
	CHECK(r, pthread_getname_np(pthread_self(), name, sizeof(name)) == 0);
	CHECK(r, strcmp(current->comm, name) == 0);
#endif
	int count = pr_warn("I915_PORT_LOG task=%s value=%d\n", current->comm, 915);
	CHECK(r, count == (int)(strlen("I915_PORT_LOG task=") + strlen(current->comm) + strlen(" value=915\n")));
	int untouched = 42;
	CHECK(r, printk("%n", &untouched) == -EINVAL && untouched == 42);
	CHECK(r, printk("%pV", (void *)NULL) == -EINVAL);
	CHECK(r, printk(KERN_CONT "unsupported continuation") == -EINVAL);
	CHECK(r, printk("%600s", "x") == -ENOSPC);
	const u32 majors[] = {0, 1, 255, 2047, 2048, 4095};
	const u32 minors[] = {0, 1, 255, 256, 0xfffff};
	for (unsigned i = 0; i < sizeof(majors)/sizeof(majors[0]); i++)
		for (unsigned j = 0; j < sizeof(minors)/sizeof(minors[0]); j++) {
			renku_linux_dev_t dev = MKDEV(majors[i], minors[j]);
			CHECK(r, MAJOR(dev) == majors[i] && MINOR(dev) == minors[j]);
			CHECK(r, new_decode_dev(new_encode_dev(dev)) == dev);
			CHECK(r, huge_decode_dev(huge_encode_dev(dev)) == dev);
			CHECK(r, old_valid_dev(dev) == (majors[i] < 256 && minors[j] < 256));
			if (old_valid_dev(dev)) CHECK(r, old_decode_dev(old_encode_dev(dev)) == dev);
		}
}
#endif
