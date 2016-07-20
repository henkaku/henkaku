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
	unsigned *argp;
	int X, Y;
	void *base;

	unsigned sysmodule_svc_offset;

	int (*sceKernelAllocMemBlock)();
	int (*sceKernelGetMemBlockBase)();
	int (*sceKernelGetMemBlockInfoByAddr)();
	int (*sceKernelGetThreadInfo)();
	int (*sceClibPrintf)();
	int (*kill_me)();
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

	int (*scePromoterUtilityInit)();
	int (*scePromoterUtilityPromotePkg)();
	int (*scePromoterUtilityGetState)();
	int (*scePromoterUtilityGetResult)();
	int (*scePromoterUtilityExit)();
} func_map;

#include "../../build/config.h"
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

#if RELEASE
#define LOG(...)
#else
#define LOG F->sceClibPrintf
#endif

// args: F, dest (in cdram), src (any)
int render_thread(int args, unsigned *argp) {
	int ret;
	func_map *F = (void*)argp[0];

	SceDisplayFrameBuf fb = {0};
	fb.size = sizeof(SceDisplayFrameBuf);
		fb.base = (void*)F->base;
	fb.pitch = SCREEN_WIDTH;
	fb.width = SCREEN_WIDTH;
	fb.height = SCREEN_HEIGHT;
	fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	while (1) {
		ret = F->sceDisplaySetFramebuf(&fb, 1);
		if (ret < 0)
				LOG("sceDisplaySetFramebuf: 0x%x\n", ret);
		F->sceKernelDelayThread(10 * 1000);
	}
}

void resolve_functions(func_map *F) {
	int ret;

	unsigned SceLibKernel_base = F->argp[0];
	unsigned SceDriverUser_base = 0;
	unsigned SceGxm_base = 0;
	unsigned SceLibHttp_base = 0;
	unsigned ScePromoterUtil_base = 0;
	unsigned SceCommonDialog_base = 0;

	F->sceKernelGetModuleList = (void*)(SceLibKernel_base + 0x675C);
	F->sceKernelGetModuleInfo = (void*)(SceLibKernel_base + 0x676C);
	F->sceClibPrintf = (void*)(SceLibKernel_base + 0x8A5D);

	int module_list[0x100];
	int num_loaded = sizeof(module_list)/sizeof(*module_list);
	ret = F->sceKernelGetModuleList(0xFF, module_list, &num_loaded);
	LOG("sceKernelGetModuleList: 0x%x, %d modules\n", ret, num_loaded);
	for (int i = 0; i < num_loaded; ++i) {
		SceKernelModuleInfo info = {0};
		info.size = sizeof(info);
		F->sceKernelGetModuleInfo(module_list[i], &info);
		char *name = info.module_name;
		unsigned addr = (unsigned)info.segments[0].vaddr;
		if (!strcmp(name, "SceDriverUser"))
			SceDriverUser_base = addr;
		else if (!strcmp(name, "SceGxm"))
			SceGxm_base = addr;
		else if (!strcmp(name, "SceLibHttp"))
			SceLibHttp_base = addr;
		else if (!strcmp(name, "ScePromoterUtil"))
			ScePromoterUtil_base = addr;
		else if (!strcmp(name, "SceCommonDialog"))
			SceCommonDialog_base = addr;
		LOG("Module %s at 0x%x\n", info.module_name, info.segments[0].vaddr);
	}

	F->sceKernelAllocMemBlock = (void*)(SceLibKernel_base + 0x610C);
	F->sceKernelGetMemBlockBase = (void*)(SceLibKernel_base + 0x60FC);
	F->sceKernelGetThreadInfo = (void*)(SceLibKernel_base + 0xA791);
	F->kill_me = (void*)(SceLibKernel_base + 0x684C);
	F->sceDisplaySetFramebuf = (void*)(SceDriverUser_base + 0x428D);
	F->sceDisplayGetFramebuf = (void*)(SceDriverUser_base + 0x42A9);
	F->sceKernelDelayThread = (void*)(SceDriverUser_base + 0xA98);
	F->sceKernelExitThread = (void*)(SceLibKernel_base + 0x61FC);
	F->sceKernelCreateThread = (void*)(SceLibKernel_base + 0xACC9);
	F->sceKernelStartThread = (void*)(SceLibKernel_base + 0xA789);
	F->sceClibVsnprintf = (void*)(SceLibKernel_base + 0x8A21);
	F->sceClibSnprintf = (void*)(SceLibKernel_base + 0x89D9);
	F->sceKernelGetMemBlockInfoByAddr = (void*)(SceGxm_base + 0x79C);
	F->sceKernelFindMemBlockByAddr = (void*)(SceLibKernel_base + 0x60DC);
	F->sceKernelFreeMemBlock = (void*)(SceLibKernel_base + 0x60EC);

	F->sceHttpCreateTemplate = (void*)(SceLibHttp_base + 0x947B);
	F->sceHttpCreateConnectionWithURL = (void*)(SceLibHttp_base + 0x950B);
	F->sceHttpCreateRequestWithURL = (void*)(SceLibHttp_base + 0x95FF);
	F->sceHttpSendRequest = (void*)(SceLibHttp_base + 0x9935);
	F->sceHttpReadData = (void*)(SceLibHttp_base + 0x9983);

	F->sceIoOpen = (void*)(SceLibKernel_base + 0xA4AD);
	F->sceIoWrite = (void*)(SceLibKernel_base + 0x68DC);
	F->sceIoClose = (void*)(SceLibKernel_base + 0x6A0C);
	F->sceIoMkdir = (void*)(SceLibKernel_base + 0xA4F5);

	F->scePromoterUtilityInit = (void*)(ScePromoterUtil_base + 0x1);
	F->scePromoterUtilityExit = (void*)(ScePromoterUtil_base + 0xF);
	F->scePromoterUtilityPromotePkg = (void*)(ScePromoterUtil_base + 0x93);
	F->scePromoterUtilityGetState = (void*)(ScePromoterUtil_base + 0x249);
	F->scePromoterUtilityGetResult = (void*)(ScePromoterUtil_base + 0x263);

	F->sysmodule_svc_offset = SceCommonDialog_base + 0xC988;
}

#define PRINTF(fmt, ...) do { psvDebugScreenPrintf(F, F->base, &F->X, &F->Y, fmt, ##__VA_ARGS__); LOG(fmt, ##__VA_ARGS__); } while (0);

// Downloads a file from url src to filesystem dst, if dst already exists, it is overwritten
void download_file(func_map *F, const char *src, const char *dst) {
	int ret;
	PRINTF("enter download file src=%s dst=%s\n", src, dst);
	int tpl = F->sceHttpCreateTemplate("henkaku usermode", 2, 1);
	LOG("create template ok\n");
	LOG("sceHttpCreateTemplate: 0x%x\n", tpl);
	int conn = F->sceHttpCreateConnectionWithURL(tpl, src, 0);
	LOG("sceHttpCreateConnectionWithURL: 0x%x\n", conn);
	int req = F->sceHttpCreateRequestWithURL(conn, 0, src, 0, 0, 0);
	LOG("sceHttpCreateRequestWithURL: 0x%x\n", req);
	ret = F->sceHttpSendRequest(req, NULL, 0);
	PRINTF("sceHttpSendRequest: 0x%x\n", ret);
	unsigned char buf[4096] = {0};

	int fd = F->sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 7);
	PRINTF("sceIoOpen: 0x%x\n", fd);
	while (1) {
		int read = F->sceHttpReadData(req, buf, sizeof(buf));
		if (read < 0) {
			PRINTF("sceHttpReadData error! 0x%x\n", read);
			break;
		}
		if (read == 0)
			break;
		PRINTF(".");
		ret = F->sceIoWrite(fd, buf, read);
		if (ret < 0 || ret != read) {
			PRINTF("sceIoWrite error! 0x%x\n", ret);
			break;
		}
		PRINTF("+");
	}
	PRINTF("\n");
	ret = F->sceIoClose(fd);
	LOG("sceIoClose: 0x%x\n", ret);
}

unsigned __attribute__((noinline)) call_syscall(unsigned a1, unsigned num) {
	__asm__ (
		"mov r12, %0 \n"
		"svc 0 \n"
		"bx lr \n"
		: : "r" (num)
	);
}

void install_pkg(func_map *F) {
	int ret;
	char dirname[32];
	char pkg_path[0x100];
	char file_name[0x100];
	// TODO: vitashell should clean up whole data/pk directory on launch
	F->sceClibSnprintf(pkg_path, sizeof(pkg_path), "ux0:data/package_temp/%x/", &install_pkg); // this is to get random directory
	LOG("package temp directory: %s\n", pkg_path);
	ret = F->sceIoMkdir("ux0:/data/package_temp", 6);
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

	unsigned *addr = (void*)F->sysmodule_svc_offset;
	unsigned syscall_num = (addr[0] & 0xFFF) + 1;

	LOG("syscall number: 0x%x\n", syscall_num);
	ret = call_syscall(SCE_SYSMODULE_PROMOTER_UTIL, syscall_num);
	PRINTF("sceSysmoduleLoadModuleInternal: 0x%x\n", ret);

	// re-resolve functions now that we've loaded promoter
	resolve_functions(F);

	ret = F->scePromoterUtilityInit();
	PRINTF("scePromoterUtilityInit: 0x%x\n", ret);

	ret = F->scePromoterUtilityPromotePkg(pkg_path, 0);
	PRINTF("scePromoterUtilityPromotePkg: 0x%x\n", ret);

	int state = 1;
	do
	{
		ret = F->scePromoterUtilityGetState(&state);
		if (ret < 0)
		{
			PRINTF("scePromoterUtilityGetState error 0x%x\n", ret);
			return;
		}
		PRINTF("scePromoterUtilityGetState status 0x%x\n", ret);
		F->sceKernelDelayThread(1000000);
	} while (state);

	int res = 0;
	ret = F->scePromoterUtilityGetResult(&res);
	PRINTF("scePromoterUtilityGetResult: ret=0x%x res=0x%x\n", ret, res);

	ret = F->scePromoterUtilityExit();
	PRINTF("scePromoterUtilityExit: 0x%x\n", ret);
}

void __attribute__ ((section (".text.start"))) user_payload(int args, unsigned *argp) {
	unsigned ret;
	struct func_map FF = {0};
	FF.argp = argp;
	resolve_functions(&FF);
	struct func_map *F = &FF;

	F->sceKernelDelayThread(1000 * 1000);

	LOG("hello from the browser!\n");

	// allocate graphics and start render thread
	int block = F->sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, FRAMEBUFFER_SIZE, NULL);
	LOG("block: 0x%x\n", block);
	ret = F->sceKernelGetMemBlockBase(block, &F->base);
	LOG("sceKernelGetMemBlockBase: 0x%x base 0x%x\n", ret, F->base);

	int thread = F->sceKernelCreateThread("", render_thread, 64, 0x1000, 0, 0, 0);
	LOG("create thread 0x%x\n", thread);

	unsigned thread_args[] = { (unsigned)F, 0x60440000, (unsigned)F->base };

	for (int i = 0; i < FRAMEBUFFER_SIZE; i += 4)
	{
		((unsigned int *)F->base)[i/4] = 0xFFFF00FF;
	}

	ret = F->sceKernelStartThread(thread, sizeof(thread_args), thread_args);

	// done with the bullshit now, let's rock
	PRINTF("this is HENkaku version " BUILD_VERSION " built at " BUILD_DATE " by " BUILD_HOST "\n");
	PRINTF("...\n");

	 F->sceKernelDelayThread(1000 * 1000);
	LOG("am still running\n");

	install_pkg(F);

	while (1) {
		F->kill_me();
		F->sceKernelDelayThread(100 * 1000);
	}
}
