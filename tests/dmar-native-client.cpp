// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "IntelDmarInventory.h"
#include "dmar-native-protocol.h"
int main()
{
	int fd=open("/dev/misc/intel_dmar_inventory",O_RDWR);
	if(fd<0) {perror("open");return 1;}
	DmarSnapshot snapshot={},cached={};
	if(ioctl(fd,DMAR_IOCTL,&snapshot,sizeof(snapshot))<0
		||ioctl(fd,DMAR_IOCTL,&cached,sizeof(cached))<0) {perror("snapshot");close(fd);return 1;}
	close(fd);
	bool same=memcmp(&snapshot,&cached,sizeof(snapshot))==0;
	printf("version=%u status=%d bytes=%u cached=%d\n",snapshot.version,int(snapshot.status),snapshot.length,int(same));
	if(snapshot.version==1 && snapshot.length>=48 && snapshot.length<=4096) {
	FILE* file=fopen("dmar-table.bin","wb");
	if(!file) {perror("save");return 1;}
	bool saved=fwrite(snapshot.bytes,1,snapshot.length,file)==snapshot.length;
	if(fclose(file)!=0||!saved) return 1;
	}
	IntelDmarInventory inventory;
	if(snapshot.version!=1||snapshot.status!=B_OK||!same
		||!intel_dmar_inventory(snapshot.bytes,snapshot.length,&inventory)) {
		puts("Inventory unavailable/invalid; DMA status UNKNOWN.");return 1;
	}

	printf("records=%u units=%u other_records=%u unknown_scopes=%u\n",inventory.records,inventory.unitCount,inventory.unknownRecords,inventory.unknownScopes);
	for(unsigned i=0;i<inventory.unitCount;i++) {
		const auto& unit=inventory.units[i];
		printf("DRHD%u segment=%u flags=%#x include_all=%u registers=%#" PRIx64 " scopes=%u\n",i,unit.segment,unit.flags,unsigned(unit.flags&1),unit.registers,unsigned(unit.scopes));
		for(size_t s=unit.offset+16;s<size_t(unit.offset)+unit.length;s+=snapshot.bytes[s+1]) {
			printf(" scope type=%u bus=%u path=",unsigned(snapshot.bytes[s]),unsigned(snapshot.bytes[s+5]));
			for(unsigned j=6;j<snapshot.bytes[s+1];j+=2) printf("%u.%u/",unsigned(snapshot.bytes[s+j]),unsigned(snapshot.bytes[s+j+1]));
			puts("");
		}
	}
	puts("Read-only firmware inventory; translation state UNKNOWN. No controller registers accessed.");
	return 0;
}
