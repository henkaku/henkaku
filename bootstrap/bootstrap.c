#include <stddef.h>
#include <stdarg.h>

#include <psp2kern/kernel/modulemgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>
#include <psp2/shellutil.h>
#include <psp2/promoterutil.h>
#include <psp2/sysmodule.h>
#include <psp2/appmgr.h>
#include "compress.h"
#include "molecule_logo.h"

#include "../build/version.c"

#define INSTALL_ATTEMPTS 3

#if RELEASE
#define LOG(...)
#else
#define LOG sceClibPrintf
#endif

#define DRAWF(fmt, ...) do { psvDebugScreenPrintf(cui_data.base, &cui_data.X, &cui_data.Y, fmt, ##__VA_ARGS__); LOG(fmt, ##__VA_ARGS__); } while (0);

uint32_t crc32(uint32_t crc, const void *buf, size_t size);

const char taihen_config[] = 
	"# You must reboot for changes to take place.\n"
	"*KERNEL\n"
	"# henkaku.skprx is hard-coded to load and is not listed here\n"
	"*main\n"
	"# main is a special titleid for SceShell\n"
	"ux0:app/MLCL00001/henkaku.suprx\n"
	"*NPXS10015\n"
	"# this is for modifying the version string\n"
	"ux0:app/MLCL00001/henkaku.suprx\n";

static int g_tpl; // http template

static struct {
	int X, Y;
	unsigned fg_color;
	void *base;
} cui_data;

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

static void clear_screen(void) {
	uint32_t *fb = (uint32_t *)cui_data.base;
	uint16_t *logo = (uint16_t *)(cui_data.base);

	for (int i = 0; i < FRAMEBUFFER_SIZE; i += 4)
	{
		((unsigned int *)cui_data.base)[i/4] = 0xFF000000;
	}

	decompress((char *)molecule_logo.data, cui_data.base, sizeof(molecule_logo.data), molecule_logo.size);

	cui_data.Y = SCREEN_HEIGHT-molecule_logo.height;
	cui_data.X = SCREEN_WIDTH-molecule_logo.width;

	for (int y = 0; y < molecule_logo.height; ++y)
	{
		for (int x = 0; x < molecule_logo.width; ++x)
		{
			uint16_t rgb = logo[x+y*molecule_logo.width];

			uint8_t r = ((((rgb >> 11) & 0x1F) * 527) + 23) >> 6;
			uint8_t g = ((((rgb >> 5) & 0x3F) * 259) + 33) >> 6;
			uint8_t b = (((rgb & 0x1F) * 527) + 23) >> 6;

			uint32_t *fb_ptr = fb + (cui_data.X + x) + (cui_data.Y + y)*LINE_SIZE;
			*fb_ptr = 0xFF000000 | (b << 16) | (g << 8) | r;
		}
	}

	// clear uncompressed data
	sceClibMemset(cui_data.base, 0, molecule_logo.size);

	cui_data.Y = 32;
	cui_data.X = 0;
}

static void printTextScreen(const char * text, uint32_t *g_vram, int *X, int *Y)
{
	int c, i, j, l;
	unsigned char *font;
	uint32_t *vram_ptr;
	uint32_t *vram;

	for (c = 0; c < sceClibStrnlen(text, 256); c++) {
		if (*X + 8 >= SCREEN_WIDTH) {
			*Y += 8;
			*X = 0;
		}
		if (*Y >= SCREEN_HEIGHT) {
			clear_screen();
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
				if ((*font & (128 >> j))) *vram_ptr = cui_data.fg_color;
				else *vram_ptr = 0;
				vram_ptr++;
			}
			vram += LINE_SIZE;
		}
		*X += 8;
	}
}

void psvDebugScreenPrintf(uint32_t *g_vram, int *X, int *Y, const char *format, ...) {
	char buf[512];

	va_list opt;
	va_start(opt, format);
	sceClibVsnprintf(buf, sizeof(buf), format, opt);
	printTextScreen(buf, g_vram, X, Y);
	va_end(opt);
}
// end draw functions

// args: F
int render_thread(SceSize args, void *argp) {
	int ret;

	SceDisplayFrameBuf fb;
	fb.size = sizeof(SceDisplayFrameBuf);
	fb.base = (void*)cui_data.base;
	fb.pitch = SCREEN_WIDTH;
	fb.width = SCREEN_WIDTH;
	fb.height = SCREEN_HEIGHT;
	fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	while (1) {
		ret = sceDisplaySetFrameBuf(&fb, 1);
		if (ret < 0)
			LOG("sceDisplaySetFrameBuf: 0x%x\n", ret);
		sceKernelDelayThread(10 * 1000);
	}
}

void draw_rect(int x, int y, int width, int height) {
	if (y >= SCREEN_HEIGHT) {
		clear_screen();
		y = 32;
	}
	for (int j = y; j < y + height; ++j)
		for (int i = x; i < x + width; ++i)
			((unsigned*)cui_data.base)[j * LINE_SIZE + i] = cui_data.fg_color;
}

// Downloads a file from url src to filesystem dst, if dst already exists, it is overwritten
int download_file(const char *src, const char *dst) {
	int ret;
	DRAWF("downloading %s\n", src);
	int conn = sceHttpCreateConnectionWithURL(g_tpl, src, 0);
	if (conn < 0) {
		DRAWF("sceHttpCreateConnectionWithURL: 0x%x\n", conn);
		return conn;
	}
	int req = sceHttpCreateRequestWithURL(conn, 0, src, 0);
	if (req < 0) {
		DRAWF("sceHttpCreateRequestWithURL: 0x%x\n", req);
		sceHttpDeleteConnection(conn);
		return req;
	}
	ret = sceHttpSendRequest(req, NULL, 0);
	if (ret < 0) {
		DRAWF("sceHttpSendRequest: 0x%x\n", ret);
		goto end;
	}
	unsigned char buf[4096];

	uint64_t length = 0;
	ret = sceHttpGetResponseContentLength(req, &length);

	int fd = sceIoOpen(dst, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
	int total_read = 0;
	if (fd < 0) {
		DRAWF("sceIoOpen: 0x%x\n", fd);
		ret = fd;
		goto end;
	}
	// draw progress bar background
	cui_data.fg_color = 0xFF666666;
	draw_rect(cui_data.X, cui_data.Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT);
	cui_data.fg_color = 0xFFFFFFFF;
	while (1) {
		int read = sceHttpReadData(req, buf, sizeof(buf));
		if (read < 0) {
			DRAWF("sceHttpReadData error! 0x%x\n", read);
			ret = read;
			goto end2;
		}
		if (read == 0)
			break;
		ret = sceIoWrite(fd, buf, read);
		if (ret < 0 || ret != read) {
			DRAWF("sceIoWrite error! 0x%x\n", ret);
			goto end2;
		}
		total_read += read;
		draw_rect(cui_data.X + 1, cui_data.Y + 1, (PROGRESS_BAR_WIDTH - 2) * total_read / length, PROGRESS_BAR_HEIGHT - 2);
	}
	DRAWF("\n\n");

end2:
	sceIoClose(fd);
end:
	sceHttpDeleteRequest(req);
	sceHttpDeleteConnection(conn);

	return ret;
}

unsigned __attribute__((naked, noinline)) call_syscall(unsigned a1, unsigned a2, unsigned a3, unsigned num) {
	__asm__ (
		"mov r12, %0 \n"
		"svc 0 \n"
		"bx lr \n"
		: : "r" (num)
	);
}

static void mkdirs(const char *dir) {
	char dir_copy[0x400];
	sceClibSnprintf(dir_copy, sizeof(dir_copy) - 2, "%s", dir);
	dir_copy[sceClibStrnlen(dir_copy,0x400)] = '/';
	char *c;
	for (c = dir_copy; *c; ++c) {
		if (*c == '/') {
			*c = '\0';
			sceIoMkdir(dir_copy, 0777);
			*c = '/';
		}
	}
}

#define GET_FILE(name) do { \
	sceClibSnprintf(file_name, sizeof(file_name), "%s/" name, pkg_path); \
	sceClibSnprintf(url, sizeof(url), "%s/" name, pkg_url_prefix); \
	download_file(url, file_name); \
} while(0);

static int exists(const char *path) {
	int fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;
	sceIoClose(fd);
	return 1;
}

/*
static int mark_world_readable(const char *path) {
	SceIoStat stat;
	int ret;
	ret = sceIoGetstat(path, &stat);
	stat.st_mode = 0x2186;
	ret = sceIoChstat(path, &stat, 1);
	return ret;
}
*/

int install_pkg(const char *pkg_url_prefix) {
	int ret;
	char pkg_path[0x400];
	char file_name[0x400];
	char url[0x400];

	LOG("package url prefix: %x\n", pkg_url_prefix);

	// this is to get random directory
	sceClibSnprintf(pkg_path, sizeof(pkg_path), "ux0:ptmp/%x", (((unsigned)&install_pkg) >> 4) * 12347);
	LOG("package temp directory: %s\n", pkg_path);

	// create directory structure
	sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys/package", pkg_path);
	mkdirs(file_name);
	sceClibSnprintf(file_name, sizeof(file_name), "%s/sce_sys/livearea/contents", pkg_path);
	mkdirs(file_name);

	GET_FILE("eboot.bin");
	GET_FILE("henkaku.skprx");
	GET_FILE("henkaku.suprx");
	GET_FILE("sce_sys/param.sfo");
	GET_FILE("sce_sys/icon0.png");
	GET_FILE("sce_sys/package/head.bin");
	GET_FILE("sce_sys/livearea/contents/bg.png");
	GET_FILE("sce_sys/livearea/contents/install_button.png");
	GET_FILE("sce_sys/livearea/contents/startup.png");
	GET_FILE("sce_sys/livearea/contents/template.xml");

	// done with downloading, let's install it now
	DRAWF("\n\ninstalling the package\n");

	ret = scePromoterUtilityInit();
	if (ret < 0) {
		DRAWF("scePromoterUtilityInit: 0x%x\n", ret);
		return ret;
	}

	ret = scePromoterUtilityPromotePkg(pkg_path, 0);
	if (ret < 0) {
		DRAWF("scePromoterUtilityPromotePkg: 0x%x\n", ret);
		return ret;
	}

	int state = 1;
	do
	{
		ret = scePromoterUtilityGetState(&state);
		if (ret < 0)
		{
			DRAWF("scePromoterUtilityGetState error 0x%x\n", ret);
			return ret;
		}
		DRAWF(".");
		sceKernelDelayThread(300000);
	} while (state);
	DRAWF("\n");

	int res = 0;
	ret = scePromoterUtilityGetResult(&res);
	if (ret < 0 || res < 0)
		DRAWF("scePromoterUtilityGetResult: ret=0x%x res=0x%x\n", ret, res);

	ret = scePromoterUtilityExit();
	if (ret < 0)
		DRAWF("scePromoterUtilityExit: 0x%x\n", ret);

	if (res < 0)
		return res;

	// create VitaShell support directory
	LOG("Creating VitaShell directory\n");
	sceIoMkdir("ux0:VitaShell", 0777);

	return ret;
}

int write_taihen_config(void) {
	int fd;

	// write default config
	sceIoRemove("ux0:tai/config.txt");
	fd = sceIoOpen("ux0:tai/config.txt", SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
	sceIoWrite(fd, taihen_config, sizeof(taihen_config) - 1);
	sceIoClose(fd);

	return 0;
}

int install_taihen(const char *pkg_url_prefix) {
	int ret;
	const char *pkg_path = "ux0:tai";
	char file_name[0x400];
	char url[0x400];

	// this is to get random directory
	LOG("taiHEN install directory: %s\n", pkg_path);

	// create directory structure
	mkdirs(pkg_path);

	sceIoRemove("ux0:tai/taihen.skprx");
	GET_FILE("taihen.skprx");

	if (!exists("ux0:tai/config.txt")) {
		write_taihen_config();
	}

	if (exists("ux0:tai/taihen.skprx")) {
		return 0;
	} else {
		return -1;
	}
}

static uint32_t crc32_file(const char *path) {
	uint32_t crc;
	char buffer[1024];
	int fd;
	int rd;
	crc = 0;
	fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0) return 0;
	while ((rd = sceIoRead(fd, buffer, 1024)) > 0) {
		crc = crc32(crc, buffer, rd);
	}
	sceIoClose(fd);
	return crc;
}

int verify_taihen(void) {
	uint32_t crc;
	if (TAIHEN_CRC32 > 0) { // 0 skips checks
		crc = crc32_file("ux0:tai/taihen.skprx");
		DRAWF("taihen.skprx CRC32: 0x%08X\n", crc);
		if (crc != TAIHEN_CRC32) return -1;
		crc = crc32_file("ux0:app/MLCL00001/henkaku.skprx");
		DRAWF("henkaku.skprx CRC32: 0x%08X\n", crc);
		if (crc != HENKAKU_CRC32) return -1;
		crc = crc32_file("ux0:app/MLCL00001/henkaku.suprx");
		DRAWF("henkaku.suprx CRC32: 0x%08X\n", crc);
		if (crc != HENKAKU_USER_CRC32) return -1;
	}
	return 1;
}

int should_reinstall(void) {
	SceCtrlData buf;
	int ret;

	ret = sceCtrlPeekBufferPositive(0, &buf, 1);
	LOG("sceCtrlPeekBufferPositive: 0x%x, 0x%x\n", ret, buf.buttons);
	if (ret < 0) {
		return 0;
	}
	if (buf.buttons & (SCE_CTRL_R1 | SCE_CTRL_RTRIGGER)) {
		return 1;
	} else {
		return 0;
	}
}

static int load_paf() {
	unsigned ptr[0x100];
	sceClibMemset(ptr, 0, sizeof(ptr));
	ptr[0] = 0;
	ptr[1] = (unsigned)&ptr[0];
	unsigned scepaf_argp[] = {0x400000, 0xEA60, 0x40000, 0, 0};
	return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_PAF, sizeof(scepaf_argp), scepaf_argp, ptr);
}

static void init_modules() {
	SceNetInitParam netInitParam;
	int ret;
	void *base;

	ret = load_paf();
	LOG("SCE_SYSMODULE_PROMOTER_UTIL(SCE_SYSMODULE_PAF): %x\n", ret);

	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_PROMOTER_UTIL);
	LOG("SCE_SYSMODULE_PROMOTER_UTIL(SCE_SYSMODULE_PROMOTER_UTIL): %x\n", ret);

	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_NET);
	LOG("SCE_SYSMODULE_PROMOTER_UTIL(SCE_SYSMODULE_NET): %x\n", ret);

	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_HTTP);
	LOG("SCE_SYSMODULE_PROMOTER_UTIL(SCE_SYSMODULE_HTTP): %x\n", ret);

	ret = sceHttpInit(1*1024*1024);
	LOG("sceHttpInit(): %x\n", ret);

	int block = sceKernelAllocMemBlock("net", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, 1*1024*1024, NULL);
	LOG("block: 0x%x\n", block);
	ret = sceKernelGetMemBlockBase(block, &base);
	LOG("sceKernelGetMemBlockBase: 0x%x base 0x%x\n", ret, base);

	netInitParam.memory = base;
	netInitParam.size = 1*1024*1024;
	netInitParam.flags = 0;
	ret = sceNetInit(&netInitParam);
	LOG("sceNetInit(): %x\n", ret);

	ret = sceNetCtlInit();
	LOG("sceNetCtlInit(): %x\n", ret);

	g_tpl = sceHttpCreateTemplate("henkaku usermode", 2, 1);
	LOG("sceHttpCreateTemplate: 0x%x\n", g_tpl);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	int ret;
	int syscall_id;
	int tries = INSTALL_ATTEMPTS;

	LOG("hello world! argc: %d\n", argc);
	syscall_id = *(uint16_t *)args;
	LOG("syscall_id: %x\n", syscall_id);

	ret = sceShellUtilInitEvents(0);
	LOG("sceShellUtilInitEvents: %x\n", ret);
	ret = sceShellUtilLock(7);
	LOG("sceShellUtilLock: %x\n", ret);

	LOG("killing parent\n");
	ret = sceAppMgrDestroyOtherApp(-2);
	LOG("sceAppMgrDestroyOtherApp: %x\n", ret);

	init_modules();

	cui_data.fg_color = 0xFFFFFFFF;
	cui_data.Y = 36; // make sure text starts below the status bar
	cui_data.X = 191;
	sceKernelDelayThread(1000 * 1000);

	// allocate graphics and start render thread
	int block = sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, FRAMEBUFFER_SIZE, NULL);
	LOG("block: 0x%x\n", block);
	ret = sceKernelGetMemBlockBase(block, &cui_data.base);
	LOG("sceKernelGetMemBlockBase: 0x%x base 0x%x\n", ret, cui_data.base);

	// draw logo
	clear_screen();

	int thread = sceKernelCreateThread("", render_thread, 64, 0x1000, 0, 0, 0);
	LOG("create thread 0x%x\n", thread);

	ret = sceKernelStartThread(thread, 0, NULL);

	// done with the bullshit now, let's rock
	DRAWF("HENkaku R%d%s (" BUILD_VERSION ") built at " BUILD_DATE "\n", HENKAKU_RELEASE, BETA_RELEASE ? " Beta" : "");
	DRAWF("Please demand a refund if you paid for this free software either on its own or as part of a bundle!\n\n");

	cui_data.fg_color = 0xFFFF00FF;
	if (should_reinstall()) {
		DRAWF("Forcing reinstall of taiHEN and molecularShell, configuration will be reset\n\n");
		sceIoRemove("ux0:temp/app_work/MLCL00001/rec/config.bin");
		sceIoRemove("ux0:app/MLCL00001/eboot.bin");
		sceIoRemove("ux0:app/MLCL00001/henkaku.suprx");
		sceIoRemove("ux0:app/MLCL00001/henkaku.skprx");
		sceIoRemove("ux0:tai/taihen.skprx");
		write_taihen_config();
	}
	cui_data.fg_color = 0xFFFFFFFF;

	DRAWF("starting...\n");

	sceKernelDelayThread(1000 * 1000);

	LOG("am still running\n");

	int crc[3] = {0};
	do {
		// check if we actually need to install the package
		if (TAIHEN_CRC32 == 0 || (crc[0] = crc32_file("ux0:tai/taihen.skprx")) != TAIHEN_CRC32) {
			DRAWF("taihen.skprx CRC32:%x, latest:%x, getting latest version...\n", crc[0], TAIHEN_CRC32);
			ret = install_taihen(PKG_URL_PREFIX);
		} else {
			DRAWF("taiHEN already installed and is the latest version\n");
			DRAWF("(if you want to force reinstall, reboot your Vita and hold R1 while running the exploit)\n");
			ret = 0;
		}
		if (ret < 0) {
			cui_data.fg_color = 0xFF0000FF;
			DRAWF("HENkaku failed to install taiHEN: error code 0x%x, retrying (%d tries left)...\n", ret, tries);
			cui_data.fg_color = 0xFFFFFFFF;
			continue;
		}
		// check if we actually need to install the package
		if (VITASHELL_CRC32 == 0 || (crc[0] = crc32_file("ux0:app/MLCL00001/eboot.bin")) != VITASHELL_CRC32 || 
			  (crc[1] = crc32_file("ux0:app/MLCL00001/henkaku.suprx")) != HENKAKU_USER_CRC32 ||
			  (crc[2] = crc32_file("ux0:app/MLCL00001/henkaku.skprx")) != HENKAKU_CRC32) {
			DRAWF("molecularShell CRC32:%x, latest:%x\n", crc[0], VITASHELL_CRC32);
			if (crc[1]) DRAWF("henkaku.suprx CRC32:%x, latest:%x\n", crc[1], HENKAKU_USER_CRC32);
			if (crc[2]) DRAWF("henkaku.skprx CRC32:%x, latest:%x\n", crc[2], HENKAKU_CRC32);
			DRAWF("Getting latest version...\n");
			ret = install_pkg(PKG_URL_PREFIX);
		} else {
			DRAWF("molecularShell already installed and is the latest version\n");
			DRAWF("(if you want to force reinstall, reboot your Vita and hold R1 while running the exploit)\n");
			ret = 0;
		}
		if (ret < 0) {
			cui_data.fg_color = 0xFF0000FF;
			DRAWF("HENkaku failed to install pkg: error code 0x%x, retrying (%d tries left)...\n", ret, tries);
			cui_data.fg_color = 0xFFFFFFFF;
			continue;
		}
		// verify installation
		if (verify_taihen() < 0) {
			cui_data.fg_color = 0xFF0000FF;
			DRAWF("ERROR: taiHENkaku files are corrupted, trying to reinstall (%d tries left)...\n", tries);
			cui_data.fg_color = 0xFFFFFFFF;
			sceIoRemove("ux0:tai/taihen.skprx");
			sceIoRemove("ux0:app/MLCL00001/henkaku.skprx");
		} else {
			break;
		}
	} while (tries-- > 0);

	DRAWF("\n");

	DRAWF("Removing temporary patches...\n");
	call_syscall(0, 0, 0, syscall_id + 1);

	if (ret >= 0) {
		DRAWF("Starting taiHEN...\n");
		ret = call_syscall(0, 0, 0, syscall_id + 0);
	} else {
		call_syscall(0, 0, 0, syscall_id + 2);
	}

	DRAWF("Cleaning up...\n");
	call_syscall(0, 0, 0, syscall_id + 3);

	DRAWF("\n\n");
	if (ret == 0x8002d013) {
		cui_data.fg_color = 0xFFFF00FF;
		DRAWF("taiHEN has already been started!\n");
		cui_data.fg_color = 0xFFFFFFFF;
		DRAWF("(press any key to exit)\n");
		SceCtrlData buf;
		while (sceCtrlPeekBufferPositive(0, &buf, 1) > 0) {
			if (buf.buttons != 0) {
				break;
			}
		}
	} else if (ret < 0) {
		cui_data.fg_color = 0xFF0000FF;
		DRAWF("HENkaku failed to install: error code 0x%x\n", ret);
		cui_data.fg_color = 0xFFFFFFFF;
		DRAWF("(press any key to exit)\n");
		SceCtrlData buf;
		while (sceCtrlPeekBufferPositive(0, &buf, 1) > 0) {
			if (buf.buttons != 0) {
				break;
			}
		}
	} else {
		cui_data.fg_color = 0xFF00FF00;
		DRAWF("HENkaku was successfully installed\n");
		cui_data.fg_color = 0xFFFFFFFF;
		DRAWF("(the application will close automatically in 3s)\n", 3);
		sceKernelDelayThread((ret < 0 ? 10 : 3) * 1000 * 1000);
	}

	ret = sceShellUtilUnlock(7);
	LOG("sceShellUtilUnlock: %x\n", ret);
	sceKernelExitProcess(0);

	return 0;
}
