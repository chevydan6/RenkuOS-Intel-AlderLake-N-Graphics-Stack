// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "vtd-native-protocol.h"
int main()
{
	int fd=open("/dev/misc/intel_vtd_observer",O_RDWR);
	if(fd<0){perror("open");return 1;}
	VtdResult result={},cached={};
	if(ioctl(fd,VTD_IOCTL,&result,sizeof(result))<0||ioctl(fd,VTD_IOCTL,&cached,sizeof(cached))<0){perror("observe");close(fd);return 1;}
	close(fd);
	bool same=memcmp(&result,&cached,sizeof(result))==0;
	printf("version=%u status=%d units=%u cached=%d\n",result.version,int(result.status),result.count,int(same));
	if(result.version!=1||result.count>16) return 1;
	for(unsigned i=0;i<result.count;i++) {
		const VtdUnitResult& u=result.units[i];
		printf("unit=%u base=%#" PRIx64 " version=%#x/%#x CAP=%#" PRIx64 " ECAP=%#" PRIx64 " GSTS=%#x/%#x TES=%s map=%d read=%d cleanup=%d\n",i,u.base,u.sample.versionBefore,u.sample.versionAfter,u.sample.capability,u.sample.extendedCapability,u.sample.statusBefore,u.sample.statusAfter,u.translation==IntelVtdDisabled?"disabled":u.translation==IntelVtdEnabled?"enabled":"UNKNOWN",int(u.mapping),int(u.reading),int(u.cleanup));
	}
	puts("Read-only interval observation; no DMA identity or GPU execution proof.");
	return result.status==B_OK&&same?0:1;
}
