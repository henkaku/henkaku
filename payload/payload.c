#include <inttypes.h>

#define NULL ((void*)0)

void payload(uint32_t sysmem_addr) {
	// find sysmem base, etc
	uint32_t sysmem_base = sysmem_addr;
	void (*debug_printlocal)(char *s, ...) = (void*)(sysmem_base + 0x1A155);

	debug_printlocal("+++ Entered kernel payload +++\n");
	debug_printlocal("second print\n");

	while(1) {}
}

void __attribute__ ((section (".text.start"), naked)) start(void) {
	__asm__ (
		"add sp, #0x1000 \n"
		"mov r0, r4      \n"
		"b payload       \n"
	);
}
