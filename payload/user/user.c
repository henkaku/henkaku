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
	int (*sceKernelGetMemBlockInfoByAddr)();
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
	int (*sceKernelFindMemBlockByAddr)();
	int (*sceKernelFreeMemBlock)();
} func_map;

#include "../libc.c"
// #include "memcpy.c"

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

#define LOG F->sceClibPrintf

// args: F, dest (in cdram), src (any)
int render_thread(int args, unsigned *argp) {
	int ret;
	func_map *F = argp[0];

	SceDisplayFrameBuf fb = {0};
	fb.size = sizeof(SceDisplayFrameBuf);
	fb.base = argp[1]; // this is SceSharedFb
	fb.pitch = SCREEN_WIDTH;
	fb.width = SCREEN_WIDTH;
	fb.height = SCREEN_HEIGHT;
	fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;	
	while (1) {
		ret = F->sceDisplaySetFramebuf(&fb, 1);
		if (ret < 0)
			LOG("sceDisplaySetFramebuf: 0x%x\n", ret);
		memcpy(fb.base, argp[2], fb.pitch * fb.height * 4);
		// F->sceKernelDelayThread(1);
	}
}

#undef LOG

#define LOG F.sceClibPrintf

void resolve_functions(unsigned *argp, func_map *F) {
	unsigned SceWebBrowser_base = argp[0]; // TODO: different WebBrowser module on retail?
	unsigned SceLibKernel_base = argp[1];
	unsigned SceDriverUser_base = argp[2];
	unsigned SceGxm_base = argp[5];

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
	F->sceKernelGetMemBlockInfoByAddr = SceGxm_base + 0x79C;
	F->sceKernelFindMemBlockByAddr = SceLibKernel_base + 0x60DC;
	F->sceKernelFreeMemBlock = SceLibKernel_base + 0x60EC;
}

#define PRINTF(fmt, ...) psvDebugScreenPrintf(&F, base, &g_X, &g_Y, fmt, #__VA_ARGS__)

void __attribute__ ((section (".text.start"))) user_payload(int args, unsigned *argp) {
	unsigned ret;
	int g_X = 0, g_Y = 0;
	struct func_map F;
	resolve_functions(argp, &F);

	F.sceKernelDelayThread(1000 * 1000);

	LOG("hello from the browser!\n");


#if 0
	ret = F.sceKernelFindMemBlockByAddr(0x60440000, 0);
	LOG("got memblock 0x%x\n", ret);

	unsigned ScePaf_base = argp[3], ScePaf_data_base = argp[4];
	LOG("paf base: 0x%x\n", ScePaf_base);
	int (*ScePafThread_F9FFA0BE)() = ScePaf_base + 0x54981;
	unsigned lock = ScePaf_data_base + 0xe428;
	// try to deadlock the main thread by obtaining a lock and never releasing it
	// for (int i = 0; i < 0x1000; ++i) {
	// 	ret = ScePafThread_F9FFA0BE(lock);
	// 	F.sceKernelDelayThread(1);
	// 	// LOG("ScePafThread_F9FFA0BE: 0x%x\n", ret);
	// }

	F.sceKernelDelayThread(1000);

	SceKernelThreadInfo thread_info = { 0 };
	thread_info.size = sizeof(thread_info);
	F.sceKernelGetThreadInfo(0x40010003, &thread_info);
	LOG("stack: 0x%x size: 0x%x\n", thread_info.stack, thread_info.stackSize);
	for (int j = 0; j < 0x10; ++j) {
		for (unsigned i = thread_info.stackSize - 0x50; i < thread_info.stackSize - 0x40; ++i) {
			unsigned *addr = (char*)thread_info.stack + i;
			*addr = F.sceKernelExitThread;
		}
	}
	// unsigned *addr = (char*)thread_info.stack + 0x3ff30 + 0x4C;
	// // try to kill the main thread by overwriting its return pointer on stack with sceKernelExitThread
	// for (int i = 0; i < 0x10000; ++i) {
	// 	*addr = F.sceKernelExitThread;
	// 	if (i % 0x100 == 0)
	// 		F.sceKernelDelayThread(1);
	// }
	// hopefully by now the thread's dead or otherwise inactive (not always the case)
#endif

	// allocate graphics and start render thread
	int block = F.sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, FRAMEBUFFER_SIZE, NULL);
	LOG("block: 0x%x\n", block);
	unsigned base = 0;
	ret = F.sceKernelGetMemBlockBase(block, &base);
	LOG("sceKernelGetMemBlockBase: 0x%x base 0x%x\n", ret, base);

	int thread = F.sceKernelCreateThread("", render_thread, 64, 0x1000, 0, 0, 0);
	LOG("create thread 0x%x\n", thread);

	unsigned thread_args[] = { &F, 0x60440000, base };
	memset(base, 0x33, FRAMEBUFFER_SIZE);
	ret = F.sceKernelStartThread(thread, sizeof(thread_args), thread_args);

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
