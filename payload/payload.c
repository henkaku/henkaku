#include <inttypes.h>

void payload(uint32_t sysmem_addr) {
	// find sysmem base, etc

	while(1) {}
}

void __attribute__ ((section (".text.start"), naked)) start(void) {
	__asm__ (
		"mov r0, r4 \n"
		"b payload  \n"
	);
}
