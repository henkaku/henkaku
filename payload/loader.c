#include <inttypes.h>

#define NULL ((void*)0)
#define BLOCK_SIZE (0x4000)

void loader(uint32_t sysmem_addr, void *second_payload) {
	uint32_t sysmem_base = sysmem_addr;
	void (*debug_print_local)(char *s, ...) = (void*)(sysmem_base + 0x1A155);
	uint32_t (*sceKernelAllocMemBlockForKernel)(const char *s, uint32_t type, uint32_t size, void *pargs) = (void*)(sysmem_base + 0xA521);
	uint32_t (*getbase_0xA841EDDA)(uint32_t blkid, void **base) = (void*)(sysmem_base + 0x1F15);
	void (*sysmem_remap)(uint32_t blkid, uint32_t type) = (void*)(sysmem_base + 0xA74D);
	void (*flush_cache)(void *addr, uint32_t size) = (void*)(sysmem_base + 0x22FCD);
	void (*sceKernelMemcpyUserToKernel)(void *dst, void *src, uint32_t sz) = (void*)(sysmem_base + 0x825D);

	char empty_string = 0;
	uint32_t block = sceKernelAllocMemBlockForKernel(&empty_string, 0x1020D006, BLOCK_SIZE, NULL);
	void *base;
	getbase_0xA841EDDA(block, &base);
	sceKernelMemcpyUserToKernel(base, second_payload, BLOCK_SIZE);
	// TODO: decrypt data
	sysmem_remap(block, 0x1020D005);
	flush_cache(base, BLOCK_SIZE);

	void (*func)(uint32_t sysmem_addr) = (void*)((uint32_t)base + 1);
	func(sysmem_base);
}
