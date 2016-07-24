#include <inttypes.h>

#define NULL ((void*)0)
#define BLOCK_SIZE (0xB000)
#define PAYLOAD_SIZE (0xA000)

unsigned char aes_key[16] = { 0x29, 0x75, 0xda, 0xbd, 0x59, 0xe5, 0x74, 0xdd, 0xec, 0x28, 0x76, 0xd6, 0x5d, 0x11, 0x08, 0x9e };

void loader(uint32_t sysmem_addr, void *second_payload) {
	uint32_t sysmem_base = sysmem_addr;
	// void (*debug_print_local)(char *s, ...) = (void*)(sysmem_base + 0x1A155);
	uint32_t (*sceKernelAllocMemBlockForKernel)(const char *s, uint32_t type, uint32_t size, void *pargs) = (void*)(sysmem_base + 0xA521);
	uint32_t (*getbase_0xA841EDDA)(uint32_t blkid, void **base) = (void*)(sysmem_base + 0x1F15);
	// void (*sysmem_remap)(uint32_t blkid, uint32_t type) = (void*)(sysmem_base + 0xA74D);
	// void (*flush_cache)(void *addr, uint32_t size) = (void*)(sysmem_base + 0x22FCD);
	void (*dacr_memcpy)() = (void*)(sysmem_base + 0x23095);
	void (*sceKernelMemcpyUserToKernel)(void *dst, void *src, uint32_t sz) = (void*)(sysmem_base + 0x825D);
	void (*aes_setkey_0xf12b6451)(void *ctx, uint32_t blocksize, uint32_t keysize, void *key) = (void*)(sysmem_base + 0x1d8d9);
	void (*aes_decrypt_0xd8678061)(void *ctx, void *src, void *dst) = (void*)(sysmem_base + 0x1baf5);

	char empty_string = 0;
	uint32_t rw_block = sceKernelAllocMemBlockForKernel(&empty_string, 0x1020D006, BLOCK_SIZE, NULL);
	uint32_t rx_block = sceKernelAllocMemBlockForKernel(&empty_string, 0x1020D005, BLOCK_SIZE, NULL);
	void *rw_base;
	void *rx_base;
	getbase_0xA841EDDA(rw_block, &rw_base);
	getbase_0xA841EDDA(rx_block, &rx_base);
	sceKernelMemcpyUserToKernel(rw_base, second_payload, PAYLOAD_SIZE);

	void *ctx = (char*)rw_base + PAYLOAD_SIZE;
	aes_setkey_0xf12b6451(ctx, 128, 128, aes_key);
	for (uint32_t i = 0; i < PAYLOAD_SIZE; i += 0x10)
		aes_decrypt_0xd8678061(ctx, (char*)rw_base + i, (char*)rw_base + i);

	dacr_memcpy(rx_base, rw_base, BLOCK_SIZE);

	__asm__ volatile(
		"mov r4, %0 \n"
		"mov sp, %1 \n"
		"mov r0, %2 \n"
		"blx r4 \n" :: "r" ((char*)rx_base + 1), "r" ((char*)rw_base + BLOCK_SIZE - 0x100), "r" (sysmem_base)
	);
}
