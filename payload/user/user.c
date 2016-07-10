#include <stddef.h>

#include <psp2/kernel/sysmem.h>

#include "promoterutil.h"
#include "sysmodule_internal.h"

struct func_map {
	int (*sceKernelAllocMemBlock)();
	int (*debug_print)();
	int (*kill_me)();
	int (*sceSysmoduleLoadModuleInternal)();
};

void *memset(void *s, int c, size_t n)
{
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	LINE_SIZE = 960,
	FRAMEBUFFER_SIZE = 2 * 1024 * 1024,
	FRAMEBUFFER_ALIGNMENT = 256 * 1024
};

#define LOG F.debug_print

void __attribute__ ((section (".text.start"))) user_payload(int args, unsigned *argp) {
	unsigned ret;

	unsigned SceWebBrowser_base = argp[0]; // TODO: different WebBrowser module on retail?
	unsigned SceLibKernel_base = argp[1];


	// resolve the functions
	struct func_map F;
	F.sceKernelAllocMemBlock = SceLibKernel_base + 0x610C;
	F.debug_print = SceWebBrowser_base + 0xC2A44;
	F.kill_me = SceLibKernel_base + 0x684C;
	F.sceSysmoduleLoadModuleInternal = SceWebBrowser_base + 0xC2AD4;

	LOG("hello from the browser!\n");

	ret = F.sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_PROMOTER_UTIL);
	LOG("sceSysmoduleLoadModuleInternal: 0x%x\n", ret);


	SceKernelAllocMemBlockOpt opt = { 0 };
	opt.size = 4 * 5;
	opt.attr = 0x00000004;
	opt.alignment = FRAMEBUFFER_ALIGNMENT;

	int block = F.sceKernelAllocMemBlock("display", 0x09408060, 2 * 1024 * 1024, &opt);
	LOG("alloc block 0x%x\n", block);

	F.kill_me();
	while (1) {}
}
