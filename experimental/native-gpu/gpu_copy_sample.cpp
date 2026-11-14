// SPDX-License-Identifier: MIT
#include "ExecutionReport.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main()
{
	int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
	if(fd<0){perror("open native GPU execution bridge");return 1;}
	native_gpu_execution_report r={};
	int status=ioctl(fd,NATIVE_GPU_EXECUTION_IOCTL,&r,sizeof(r));close(fd);
	printf("native_copy version=%u ioctl=%d status=%d stage=%u identity=%08x revision=%u\n",r.version,status,r.status,r.stage,r.identity,r.revision);
	printf("mapping=%d dma=%d tables=%d bootstrap=%d group=%d reset=%d engine=%d csb=%d pages=%u retained=%u\n",r.mapping,r.dma,r.tables,r.bootstrap,r.group,r.reset,r.engine,r.csb,r.pages,r.retained);
	printf("nonce=%08x observed=%08x complement=%08x sequence=%u observed_sequence=%u load=%u tail=%u\n",r.nonce,r.observed_nonce,r.observed_complement,r.sequence,r.observed_sequence,r.load_attempted,r.ring_tail);
	printf("copy_words=%u matched=%u source_hash=%08x destination_hash=%08x CSB=%016llx/%016llx\n",r.copy_words,r.copy_matched,r.source_hash,r.destination_hash,(unsigned long long)r.activation,(unsigned long long)r.completion);
	printf("bootstrap=%u/%u group=%u/%u reset=%u/%u engine=%u/%u csb_error=%u safety=%u/%u/%u mode=%08x EIR=%08x fault=%08x\n",r.bootstrap_stage,r.bootstrap_error,r.group_stage,r.group_error,r.reset_stage,r.reset_error,r.engine_stage,r.engine_error,r.csb_error,r.safety_reason,r.safety_phase,r.safety_domain,r.mode,r.error_register,r.fault_register);
	printf("PCI result=%u command=%04x/%04x status=%04x/%04x PM=%04x/%04x group_domain=%u reads=%u writes=%u\n",r.pci_result,r.expected_command,r.observed_command,r.expected_status,r.observed_status,r.expected_pm,r.observed_pm,r.group_domain,r.group_reads,r.group_writes);
	printf("CSB tail=%08x raw=%016llx/%016llx/%016llx/%016llx\n",r.csb_tail_raw,(unsigned long long)r.csb_words[0],(unsigned long long)r.csb_words[1],(unsigned long long)r.csb_words[2],(unsigned long long)r.csb_words[3]);
	bool passed=status==0 && r.version==1 && r.status==0 && r.stage==14 && r.load_attempted==1
		&& r.observed_nonce==r.nonce && r.observed_complement==~r.nonce && r.sequence!=0 && r.observed_sequence==r.sequence
		&& r.copy_words==1024 && r.copy_matched==1024 && r.source_hash==r.destination_hash;
	puts(passed?"GPU 4KiB copy sample: PASS":"GPU 4KiB copy sample: NOT PROVEN");
	puts("Successful requests retire for reuse; uncertain requests retain backing until reboot.");
	return passed?0:1;
}
