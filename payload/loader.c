#include <inttypes.h>

void loader(uint32_t sysmem_addr) {
	uint32_t sysmem_base = sysmem_addr;
	void (*debug_print_local)(char *s, ...) = (void*)(sysmem_base + 0x1A155); // 3.60

	debug_print_local("entered kernel loader\n");

	while(1) {}
}
