// SPDX-License-Identifier: MIT
#include <OS.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "ppgtt-native-protocol.h"
int main()
{
	int fd=open("/dev/misc/intel_ppgtt_reserved_test",O_RDWR);
	if(fd<0) {perror("open");return 1;}
	PpgttTestResult first={},second={};
	if(ioctl(fd,PPGTT_TEST_IOCTL,&first,sizeof(first))<0) {perror("run");close(fd);return 1;}
	if(ioctl(fd,PPGTT_TEST_IOCTL,&second,sizeof(second))<0) {perror("cached");close(fd);return 1;}
	close(fd);
	printf("version=%u status=%d completed=%u stage=%u case=%u verified=%u absent=%u state=%u live=%u ranges=%u excluded_checked=%u provider=%d\n",
		first.version,int(first.status),first.completed,first.stage,first.current_case,
		first.verified_areas,first.absent_areas,first.tree_state,first.live_areas,first.reserved_ranges,first.reserved_checked,int(first.provider_error));
	bool same=memcmp(&first,&second,sizeof(first))==0;
	printf("cached_result=%s\n",same?"identical":"DIFFERENT");
	bool pass=first.version==2&&first.reserved_ranges>=1&&first.status==B_OK&&first.completed==3
		&&first.verified_areas==42&&first.reserved_checked==42&&first.live_areas==0&&first.tree_state==0&&same;
	puts(pass?"Real kernel wired-page construction/cleanup passed; no GPU execution.":"Kernel page test FAILED; retain logs and module state.");
	return pass?0:1;
}
