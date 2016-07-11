#include <stddef.h>
#include <stdarg.h>

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/display.h>

#include "promoterutil.h"
#include "sysmodule_internal.h"

#include "../../build/version.c"

typedef struct func_map {
	int (*sceKernelAllocMemBlock)();
	int (*sceKernelGetMemBlockBase)();
	int (*sceKernelGetThreadInfo)();
	int (*sceClibPrintf)();
	int (*kill_me)();
	int (*sceSysmoduleLoadModuleInternal)();
	int (*sceDisplaySetFramebuf)();
	int (*sceDisplayGetFramebuf)();
	int (*sceKernelDelayThread)();
	int (*sceKernelExitThread)();
	int (*sceKernelCreateThread)();
	int (*sceKernelStartThread)();
	int (*sceClibVsnprintf)();
} func_map;

#include "../libc.c"

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	LINE_SIZE = 960,
	FRAMEBUFFER_SIZE = 2 * 1024 * 1024,
	FRAMEBUFFER_ALIGNMENT = 256 * 1024
};

// Draw functions
#include "font.c"

static void printTextScreen(const char * text, uint32_t *g_vram, int *X, int *Y)
{
	int c, i, j, l;
	unsigned char *font;
	uint32_t *vram_ptr;
	uint32_t *vram;

	for (c = 0; c < strlen(text); c++) {
		if (*X + 8 > SCREEN_WIDTH) {
			*Y += 8;
			*X = 0;
		}
		if (*Y + 8 > SCREEN_HEIGHT) {
			*Y = 0;
			// psvDebugScreenClear(g_bg_color);
		}
		char ch = text[c];
		if (ch == '\n') {
			*X = 0;
			*Y += 8;
			continue;
		} else if (ch == '\r') {
			*X = 0;
			continue;
		}

		vram = g_vram + (*X) + (*Y) * LINE_SIZE;

		font = &msx[ (int)ch * 8];
		for (i = l = 0; i < 8; i++, l += 8, font++) {
			vram_ptr  = vram;
			for (j = 0; j < 8; j++) {
				if ((*font & (128 >> j))) *vram_ptr = 0xFFFFFFFF;
				else *vram_ptr = 0;
				vram_ptr++;
			}
			vram += LINE_SIZE;
		}
		*X += 8;
	}
}

void psvDebugScreenPrintf(func_map *F, uint32_t *g_vram, int *X, int *Y, const char *format, ...) {
	char buf[512];

	va_list opt;
	va_start(opt, format);
	F->sceClibVsnprintf(buf, sizeof(buf), format, opt);
	printTextScreen(buf, g_vram, X, Y);
	va_end(opt);
}
// end draw functions

// args: F, dest (in cdram), src (any)
int render_thread(int args, unsigned *argp) {
	func_map *F = argp[0];

	SceDisplayFrameBuf fb = {0};
	fb.size = sizeof(SceDisplayFrameBuf);
	fb.base = argp[1]; // this is SceSharedFb
	fb.pitch = SCREEN_WIDTH;
	fb.width = SCREEN_WIDTH;
	fb.height = SCREEN_HEIGHT;
	fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;	
	while (1) {
		F->sceDisplaySetFramebuf(&fb, 1);
		memcpy(fb.base, argp[2], fb.pitch * fb.height * 4);
		F->sceKernelDelayThread(1000);
	}
}

#define LOG F.sceClibPrintf

void resolve_functions(unsigned *argp, func_map *F) {
	unsigned SceWebBrowser_base = argp[0]; // TODO: different WebBrowser module on retail?
	unsigned SceLibKernel_base = argp[1];
	unsigned SceDriverUser_base = argp[2];

	F->sceKernelAllocMemBlock = SceLibKernel_base + 0x610C;
	F->sceKernelGetMemBlockBase = SceLibKernel_base + 0x60FC;
	F->sceKernelGetThreadInfo = SceLibKernel_base + 0xA791;
	F->sceClibPrintf = SceLibKernel_base + 0x8A5D;
	F->kill_me = SceLibKernel_base + 0x684C;
	F->sceSysmoduleLoadModuleInternal = SceWebBrowser_base + 0xC2AD4;
	F->sceDisplaySetFramebuf = SceDriverUser_base + 0x428D;
	F->sceDisplayGetFramebuf = SceDriverUser_base + 0x42A9;
	F->sceKernelDelayThread = SceDriverUser_base + 0xA98;
	F->sceKernelExitThread = SceLibKernel_base + 0x61FC;
	F->sceKernelCreateThread = SceLibKernel_base + 0xACC9;
	F->sceKernelStartThread = SceLibKernel_base + 0xA789;
	F->sceClibVsnprintf = SceLibKernel_base + 0x8A21;
}

#define PRINTF(fmt, ...) psvDebugScreenPrintf(&F, base, &g_X, &g_Y, fmt, #__VA_ARGS__)

void __attribute__ ((section (".text.start"))) user_payload(int args, unsigned *argp) {
	unsigned ret;
	int g_X = 0, g_Y = 0;
	struct func_map F;
	resolve_functions(argp, &F);

	F.sceKernelDelayThread(1000 * 1000);

	LOG("hello from the browser!\n");

	int block = F.sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, FRAMEBUFFER_SIZE, NULL);
	LOG("block: 0x%x\n", block);
	unsigned base = 0;
	ret = F.sceKernelGetMemBlockBase(block, &base);
	LOG("sceKernelGetMemBlockBase: 0x%x base 0x%x\n", ret, base);

	int thread = F.sceKernelCreateThread("", render_thread, 0x10000100, 0x1000, 0, 0, 0);
	LOG("create thread 0x%x\n", thread);

	unsigned thread_args[] = { &F, 0x60440000, base };
	memset(base, 0x33, FRAMEBUFFER_SIZE);
	ret = F.sceKernelStartThread(thread, sizeof(thread_args), thread_args);

	unsigned ScePaf_base = argp[3], ScePaf_data_base = argp[4];
	LOG("paf base: 0x%x\n", ScePaf_base);
	int (*ScePafThread_F9FFA0BE)() = ScePaf_base + 0x54981;
	unsigned lock = ScePaf_data_base + 0xe428;
	for (int i = 0; i < 0x10; ++i) {
		ret = ScePafThread_F9FFA0BE(lock);
		LOG("ScePafThread_F9FFA0BE: 0x%x\n", ret);
	}

	// done with the bullshit now, let's rock
	PRINTF("this is HENkaku version " BUILD_VERSION " built at " BUILD_DATE " by " BUILD_HOST "\n");
	PRINTF("...\n");

	F.sceKernelDelayThread(1000 * 1000);
	LOG("am still running\n");

	// ret = F.sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_PROMOTER_UTIL);
	// LOG("sceSysmoduleLoadModuleInternal: 0x%x\n", ret);
	
	F.sceKernelDelayThread(5 * 1000 * 1000);

	F.kill_me();
	while (1) {}
}
