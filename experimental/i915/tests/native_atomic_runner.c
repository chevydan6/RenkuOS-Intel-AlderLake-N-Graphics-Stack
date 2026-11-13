/* SPDX-License-Identifier: MIT */
#include <stdio.h>
extern int renku_native_atomic_probe(void);
int main(void)
{
	int error = renku_native_atomic_probe();
	printf("Host execution of target-header atomic code: %s (line=%d)\n",
		error ? "FAIL" : "PASS", error);
	return error != 0;
}
