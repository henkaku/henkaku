#include <stddef.h>
#include <stdarg.h>

#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/display.h>
#include <psp2/io/fcntl.h>

#include "promoterutil.h"
#include "sysmodule_internal.h"
#include "compress.h"
#include "molecule_logo.h"

#include "../../build/version.c"
#include "../args.h"

typedef struct func_map {
	struct args *args;
	int X, Y;
	unsigned fg_color;
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
	int (*sceHttpGetResponseContentLength)();

	int (*sceIoOpen)();
	int (*sceIoWrite)();
	int (*sceIoClose)();
	int (*sceIoMkdir)();
	int (*sceIoRead)();

	int (*sceKernelGetModuleList)();
	int (*sceKernelGetModuleInfo)();

	int (*scePromoterUtilityInit)();
	int (*scePromoterUtilityPromotePkg)();
	int (*scePromoterUtilityGetState)();
	int (*scePromoterUtilityGetResult)();
	int (*scePromoterUtilityExit)();
} func_map;

#include "../libc.c"
// #include "memcpy.c"

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	LINE_SIZE = 960,
	FRAMEBUFFER_SIZE = 2 * 1024 * 1024,
	FRAMEBUFFER_ALIGNMENT = 256 * 1024,
	PROGRESS_BAR_WIDTH = 400,
	PROGRESS_BAR_HEIGHT = 10,
};

// Draw functions
#include "font.c"

static void printTextScreen(func_map *F, const char * text, uint32_t *g_vram, int *X, int *Y)
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
				if ((*font & (128 >> j))) *vram_ptr = F->fg_color;
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
	printTextScreen(F, buf, g_vram, X, Y);
	va_end(opt);
}
// end draw functions

#if RELEASE
#define LOG(...)
#else
#define LOG F->sceClibPrintf
#endif

// args: F
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

	unsigned SceLibKernel_base = F->args->libbase;
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
	F->sceHttpGetResponseContentLength = (void*)(SceLibHttp_base + 0x99D7);

	F->sceIoOpen = (void*)(SceLibKernel_base + 0xA4AD);
	F->sceIoWrite = (void*)(SceLibKernel_base + 0x68DC);
	F->sceIoClose = (void*)(SceLibKernel_base + 0x6A0C);
	F->sceIoMkdir = (void*)(SceLibKernel_base + 0xA4F5);
	F->sceIoRead = (void*)(SceLibKernel_base + 0x6A9C);

	F->scePromoterUtilityInit = (void*)(ScePromoterUtil_base + 0x1);
	F->scePromoterUtilityExit = (void*)(ScePromoterUtil_base + 0xF);
	F->scePromoterUtilityPromotePkg = (void*)(ScePromoterUtil_base + 0x93);
	F->scePromoterUtilityGetState = (void*)(ScePromoterUtil_base + 0x249);
	F->scePromoterUtilityGetResult = (void*)(ScePromoterUtil_base + 0x263);

	F->sysmodule_svc_offset = SceCommonDialog_base + 0xC988;
}

#define PRINTF(fmt, ...) do { psvDebugScreenPrintf(F, F->base, &F->X, &F->Y, fmt, ##__VA_ARGS__); LOG(fmt, ##__VA_ARGS__); } while (0);

void draw_rect(func_map *F, int x, int y, int width, int height) {
	for (int j = y; j < y + height; ++j)
		for (int i = x; i < x + width; ++i)
			((unsigned*)F->base)[j * LINE_SIZE + i] = F->fg_color;
}

// Downloads a file from url src to filesystem dst, if dst already exists, it is overwritten
int download_file(func_map *F, const char *src, const char *dst) {
	int ret;
	PRINTF("downloading %s\n", src);
	int tpl = F->sceHttpCreateTemplate("henkaku usermode", 2, 1);
	if (tpl < 0) {
		PRINTF("sceHttpCreateTemplate: 0x%x\n", tpl);
		return tpl;
	}
	int conn = F->sceHttpCreateConnectionWithURL(tpl, src, 0);
	if (conn < 0) {
		PRINTF("sceHttpCreateConnectionWithURL: 0x%x\n", conn);
		return conn;
	}
	int req = F->sceHttpCreateRequestWithURL(conn, 0, src, 0, 0, 0);
	if (req < 0) {
		PRINTF("sceHttpCreateRequestWithURL: 0x%x\n", req);
		return req;
	}
	ret = F->sceHttpSendRequest(req, NULL, 0);
	if (ret < 0) {
		PRINTF("sceHttpSendRequest: 0x%x\n", ret);
		return ret;
	}
	unsigned char buf[4096] = {0};

	long long length = 0;
	ret = F->sceHttpGetResponseContentLength(req, &length);

	int fd = F->sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
	int total_read = 0;
	if (fd < 0) {
		PRINTF("sceIoOpen: 0x%x\n", fd);
		return fd;
	}
	// draw progress bar background
	F->fg_color = 0xFF666666;
	draw_rect(F, F->X, F->Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT);
	F->fg_color = 0xFFFFFFFF;
	while (1) {
		int read = F->sceHttpReadData(req, buf, sizeof(buf));
		if (read < 0) {
			PRINTF("sceHttpReadData error! 0x%x\n", read);
			return read;
		}
		if (read == 0)
			break;
		ret = F->sceIoWrite(fd, buf, read);
		if (ret < 0 || ret != read) {
			PRINTF("sceIoWrite error! 0x%x\n", ret);
			if (ret < 0)
				return ret;
			return -1;
		}
		total_read += read;
		draw_rect(F, F->X + 1, F->Y + 1, (PROGRESS_BAR_WIDTH - 2) * total_read / length, PROGRESS_BAR_HEIGHT - 2);
	}
	PRINTF("\n\n");
	ret = F->sceIoClose(fd);
	if (ret < 0)
		PRINTF("sceIoClose: 0x%x\n", ret);

	return 0;
}

unsigned __attribute__((noinline)) call_syscall(unsigned a1, unsigned num) {
	__asm__ (
		"mov r12, %0 \n"
		"svc 0 \n"
		"bx lr \n"
		: : "r" (num)
	);
}

static void mkdirs(func_map *F, const char *dir) {
	char dir_copy[0x400] = {0};
	F->sceClibSnprintf(dir_copy, sizeof(dir_copy) - 2, "%s", dir);
	dir_copy[strlen(dir_copy)] = '/';
	char *c;
	for (c = dir_copy; *c; ++c) {
		if (*c == '/') {
			*c = '\0';
			F->sceIoMkdir(dir_copy, 0777);
			*c = '/';
		}
	}
}

#define GET_FILE(name) do { \
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/" name, pkg_path); \
	F->sceClibSnprintf(url, sizeof(url), "%s/" name, pkg_url_prefix); \
	download_file(F, url, file_name); \
} while(0);

#define VERSION_TXT "ux0:app/MLCL00001/version.txt"

unsigned get_version(func_map *F) {
	int fd = F->sceIoOpen(VERSION_TXT, SCE_O_RDONLY);
	if (fd < 0)
		return 0;
	unsigned ver = 0;
	F->sceIoRead(fd, &ver, sizeof(ver));
	F->sceIoClose(fd);
	return ver;
}

void set_version(func_map *F, unsigned version) {
	int fd = F->sceIoOpen(VERSION_TXT, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
	if (fd < 0) {
		PRINTF("Failed to open version.txt: 0x%x\n", fd);
		return;
	}
	int ret = F->sceIoWrite(fd, &version, sizeof(version));
	if (ret != sizeof(version)) {
		PRINTF("Failed to write version.txt: 0x%x\n", ret);
		return;
	}
	F->sceIoClose(fd);
}

int install_pkg(func_map *F) {
	int ret;
	const char *pkg_url_prefix;
	char pkg_path[0x400] = {0};
	char file_name[0x400] = {0};
	char url[0x400] = {0};

	pkg_url_prefix = F->args->pkg_url_prefix;
	LOG("package url prefix: %x\n", pkg_url_prefix);

	// this is to get random directory
	F->sceClibSnprintf(pkg_path, sizeof(pkg_path), "ux0:ptmp/%x", (((unsigned)&install_pkg) >> 4) * 12347);
	LOG("package temp directory: %s\n", pkg_path);

	// create directory structure
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys/package", pkg_path);
	mkdirs(F, file_name);
	F->sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys/livearea/contents", pkg_path);
	mkdirs(F, file_name);

	GET_FILE("eboot.bin");
	GET_FILE("sce_sys/param.sfo");
	GET_FILE("sce_sys/icon0.png");
	GET_FILE("sce_sys/package/head.bin");
	GET_FILE("sce_sys/livearea/contents/bg.png");
	GET_FILE("sce_sys/livearea/contents/install_button.png");
	GET_FILE("sce_sys/livearea/contents/startup.png");
	GET_FILE("sce_sys/livearea/contents/template.xml");

	// done with downloading, let's install it now
	PRINTF("\n\ninstalling the package\n");

	unsigned *addr = (void*)F->sysmodule_svc_offset;
	unsigned syscall_num = (addr[0] & 0xFFF) + 1;

	LOG("syscall number: 0x%x\n", syscall_num);
	ret = call_syscall(SCE_SYSMODULE_PROMOTER_UTIL, syscall_num);
	if (ret < 0) {
		PRINTF("sceSysmoduleLoadModuleInternal: 0x%x\n", ret);
		return ret;
	}

	// re-resolve functions now that we've loaded promoter
	resolve_functions(F);

	ret = F->scePromoterUtilityInit();
	if (ret < 0) {
		PRINTF("scePromoterUtilityInit: 0x%x\n", ret);
		return ret;
	}

	ret = F->scePromoterUtilityPromotePkg(pkg_path, 0);
	if (ret < 0) {
		PRINTF("scePromoterUtilityPromotePkg: 0x%x\n", ret);
		return ret;
	}

	int state = 1;
	do
	{
		ret = F->scePromoterUtilityGetState(&state);
		if (ret < 0)
		{
			PRINTF("scePromoterUtilityGetState error 0x%x\n", ret);
			return ret;
		}
		PRINTF(".");
		F->sceKernelDelayThread(300000);
	} while (state);
	PRINTF("\n");

	int res = 0;
	ret = F->scePromoterUtilityGetResult(&res);
	if (ret < 0 || res < 0)
		PRINTF("scePromoterUtilityGetResult: ret=0x%x res=0x%x\n", ret, res);

	ret = F->scePromoterUtilityExit();
	if (ret < 0)
		PRINTF("scePromoterUtilityExit: 0x%x\n", ret);

	if (res < 0)
		return res;
	return ret;
}

void __attribute__ ((section (".text.start"))) user_payload(int args, unsigned *argp) {
	int ret;
	struct func_map FF = {0};
	FF.args = (struct args *)argp;
	resolve_functions(&FF);
	struct func_map *F = &FF;

	F->fg_color = 0xFFFFFFFF;
	F->Y = 36; // make sure text starts below the status bar
	F->X = 191;
	F->sceKernelDelayThread(1000 * 1000);

	LOG("hello from the browser!\n");

	// allocate graphics and start render thread
	int block = F->sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, FRAMEBUFFER_SIZE, NULL);
	LOG("block: 0x%x\n", block);
	ret = F->sceKernelGetMemBlockBase(block, &F->base);
	LOG("sceKernelGetMemBlockBase: 0x%x base 0x%x\n", ret, F->base);

	unsigned thread_args[] = { (unsigned)F };

	for (int i = 0; i < FRAMEBUFFER_SIZE; i += 4)
	{
		((unsigned int *)F->base)[i/4] = 0xFF000000;
	}

	// draw logo
	uint32_t *fb = (uint32_t *)F->base;
	decompress(molecule_logo.data, F->base, sizeof(molecule_logo.data), molecule_logo.size);
	uint16_t *logo = (uint16_t *)(F->base);

	F->Y = SCREEN_HEIGHT-molecule_logo.height;
	F->X = SCREEN_WIDTH-molecule_logo.width;

	for (int y = 0; y < molecule_logo.height; ++y)
	{
		for (int x = 0; x < molecule_logo.width; ++x)
		{
			uint16_t rgb = logo[x+y*molecule_logo.width];

			uint8_t r = ((((rgb >> 11) & 0x1F) * 527) + 23) >> 6;
			uint8_t g = ((((rgb >> 5) & 0x3F) * 259) + 33) >> 6;
			uint8_t b = (((rgb & 0x1F) * 527) + 23) >> 6;

			uint32_t *fb_ptr = fb + (F->X + x) + (F->Y + y)*LINE_SIZE;
			*fb_ptr = 0xFF000000 | (b << 16) | (g << 8) | r;
		}
	}

	// clear uncompressed data
	memset(F->base, 0, molecule_logo.size);

	int thread = F->sceKernelCreateThread("", render_thread, 64, 0x1000, 0, 0, 0);
	LOG("create thread 0x%x\n", thread);

	ret = F->sceKernelStartThread(thread, sizeof(thread_args), thread_args);

	F->Y = 32;
	F->X = 0;

	// done with the bullshit now, let's rock
	PRINTF("this is HENkaku version " BUILD_VERSION " built at " BUILD_DATE " by " BUILD_HOST "\n");
	PRINTF("...\n");

	F->sceKernelDelayThread(1000 * 1000);
	LOG("am still running\n");

	// check if we actually need to install the package
	if (VERSION == 0 || get_version(F) < VERSION) {
		ret = install_pkg(F);
		set_version(F, VERSION);
	} else {
		PRINTF("molecularShell already installed and is the latest version\n");
		PRINTF("(if you want to force reinstall, remove its bubble and restart the exploit)\n");
		ret = 0;
	}

	PRINTF("\n\n");
	if (ret < 0) {
		F->fg_color = 0xFF0000FF;
		PRINTF("HENkaku failed to install the pkg: error code 0x%x\n", ret);
	} else {
		F->fg_color = 0xFF00FF00;
		PRINTF("HENkaku was successfully installed\n");
	}
	F->fg_color = 0xFFFFFFFF;
	PRINTF("(the browser will close in 6s)\n");
	F->sceKernelDelayThread(6 * 1000 * 1000);

	while (1) {
		F->kill_me();
		F->sceKernelDelayThread(100 * 1000);
	}
}
