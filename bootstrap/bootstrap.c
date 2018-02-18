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
#include "../plugin/henkaku.h"

#define INSTALL_ATTEMPTS 3
#define TAIHEN_CONFIG_FILE "ux0:tai/config.txt"
#define TAIHEN_RECOVERY_CONFIG_FILE "ur0:tai/config.txt"
#define TAIHEN_SKPRX_FILE "ur0:tai/taihen.skprx"
#define HENKAKU_SUPRX_FILE "ur0:tai/henkaku.suprx"
#define HENKAKU_SKPRX_FILE "ur0:tai/henkaku.skprx"

#undef LOG
#if RELEASE
#define LOG(...)
#else
#define LOG sceClibPrintf
#endif

#define DRAWF(fmt, ...) do { psvDebugScreenPrintf(cui_data.base, &cui_data.X, &cui_data.Y, fmt, ##__VA_ARGS__); LOG(fmt, ##__VA_ARGS__); } while (0);

uint32_t crc32(uint32_t crc, const void *buf, size_t size);
typedef int (*search_fsm_t)(const char *data, int len, int state);

const char taihen_config_recovery_header[] =
	"# This file is used as an alternative if ux0:tai/config.txt is not found.\n";

const char taihen_config_updated_msg[] =
	"# henkaku.suprx and henkaku.skprx has moved to ur0:tai, you may remove\n"
	"# the old entries above.\n";

const char taihen_config_header[] = 
	"# For users plugins, you must refresh taiHEN from HENkaku Settings for\n"
	"# changes to take place.\n"
	"# For kernel plugins, you must reboot for changes to take place.\n";

const char taihen_config[] = 
	"*KERNEL\n"
	"# henkaku.skprx is hard-coded to load and is not listed here\n"
	"*main\n"
	"# main is a special titleid for SceShell\n"
	HENKAKU_SUPRX_FILE "\n"
	"*NPXS10015\n"
	"# this is for modifying the version string\n"
	HENKAKU_SUPRX_FILE "\n"
	"*NPXS10016\n"
	"# this is for modifying the version string in settings widget\n"
	HENKAKU_SUPRX_FILE "\n";

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

	cui_data.Y = 0;
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
	// ensure it's filled when file is downloaded
	draw_rect(cui_data.X + 1, cui_data.Y + 1, (PROGRESS_BAR_WIDTH - 2), PROGRESS_BAR_HEIGHT - 2);
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

static int dev_exists(const char *dev) {
	int fd = sceIoOpen(dev, SCE_O_RDONLY, 0);
	if (fd == 0x80010013)
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

int write_taihen_config(const char *path, int recovery) {
	int fd;

	// write default config
	sceIoRemove(path);
	fd = sceIoOpen(path, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
	if (recovery) {
		sceIoWrite(fd, taihen_config_recovery_header, sizeof(taihen_config_recovery_header) - 1);
	}
	sceIoWrite(fd, taihen_config_header, sizeof(taihen_config_header) - 1);
	sceIoWrite(fd, taihen_config, sizeof(taihen_config) - 1);
	sceIoClose(fd);

	return 0;
}

int install_taihen(const char *pkg_url_prefix) {
	int ret;
	const char *pkg_path = "ur0:tai";
	char file_name[0x400];
	char url[0x400];

	// this is to get random directory
	LOG("taiHEN install directory: %s\n", pkg_path);

	// create directory structure
	mkdirs(pkg_path);

	sceIoRemove(TAIHEN_SKPRX_FILE);
	sceIoRemove(HENKAKU_SKPRX_FILE);
	sceIoRemove(HENKAKU_SUPRX_FILE);
	GET_FILE("taihen.skprx");
	GET_FILE("henkaku.skprx");
	GET_FILE("henkaku.suprx");

	if (!exists(TAIHEN_CONFIG_FILE)) {
		mkdirs("ux0:tai");
		write_taihen_config(TAIHEN_CONFIG_FILE, 0);
	}

	if (!exists(TAIHEN_RECOVERY_CONFIG_FILE)) {
		write_taihen_config(TAIHEN_RECOVERY_CONFIG_FILE, 1);
	}

	if (exists(TAIHEN_SKPRX_FILE) && exists(HENKAKU_SKPRX_FILE) && exists(HENKAKU_SUPRX_FILE)) {
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

static int find_henkaku_plugin_fsm(const char *data, int len, int state) {
	int i = 0;
	while (i < len) {
		switch (state) {
			case  0: state = (data[i++] == 'u') ? 1 : 0; break;
			case  1: state = (data[i++] == 'r') ? 2 : 0; break;
			case  2: state = (data[i++] == '0') ? 3 : 0; break;
			case  3: state = (data[i++] == ':') ? 4 : 0; break;
			case  4: state = (data[i++] == 't') ? 5 : 0; break;
			case  5: state = (data[i++] == 'a') ? 6 : 0; break;
			case  6: state = (data[i++] == 'i') ? 7 : 0; break;
			case  7: state = (data[i++] == '/') ? 8 : 0; break;
			case  8: state = (data[i++] == 'h') ? 9 : 0; break;
			case  9: state = (data[i++] == 'e') ? 10 : 0; break;
			case 10: state = (data[i++] == 'n') ? 11 : 0; break;
			case 11: state = (data[i++] == 'k') ? 12 : 0; break;
			case 12: state = (data[i++] == 'a') ? 13 : 0; break;
			case 13: state = (data[i++] == 'k') ? 14 : 0; break;
			case 14: state = (data[i++] == 'u') ? 15 : 0; break;
			case 15: state = (data[i++] == '.') ? 16 : 0; break;
			case 16: state = (data[i++] == 's') ? 17 : 0; break;
			case 17: state = (data[i++] == 'u' || data[i++] == 'k') ? 18 : 0; break;
			case 18: state = (data[i++] == 'p') ? 19 : 0; break;
			case 19: state = (data[i++] == 'r') ? 20 : 0; break;
			case 20: state = (data[i++] == 'x') ? 99 : 0; break;
			case 99: state = (data[i++] == '\r' || data[i-1] == '\n') ? 100 : 0; break;
			default: i++; break;
		}
	}
	return state;
}

static int find_null_char_fsm(const char *data, int len, int state) {
	for (int i = 0; i < len; i++) {
		if (state == 100) {
			state = 100;
		} else if (data[i] == '\0') {
			state = 100;
		} else {
			state = 0;
		}
	}
	return state;
}

static int search_file_with_fsms(const char *file, int count, const search_fsm_t fsm[count], int state[count]) {
	char buf[256];
	int len;
	int fd;
	int found;

	fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0) {
		return -1;
	}
	found = 0;
	while ((len = sceIoRead(fd, buf, 256)) > 0) {
		for (int i = 0; i < count; i++) {
			if ((state[i] = fsm[i](buf, len, state[i])) >= 99) {
				found++;
			}
		}
		if (found == count) {
			break;
		}
	}
	sceIoClose(fd);

	return found;
}

int update_taihen_config(void) {
	int fd;
	static const search_fsm_t fsm[2] = {find_null_char_fsm, find_henkaku_plugin_fsm};
	int state[2] = {0};
	int ret;

	if ((ret = search_file_with_fsms(TAIHEN_CONFIG_FILE, 2, fsm, state)) >= 0) {
		if (state[0] >= 99) { // found null char
			DRAWF("ux0:tai/config.txt is corrupted, rewriting it\n");
			write_taihen_config(TAIHEN_CONFIG_FILE, 0);
		} else if (state[1] < 99) { // ur0 path not found
			DRAWF("Updating taiHEN config.txt\n");
			fd = sceIoOpen(TAIHEN_CONFIG_FILE, SCE_O_WRONLY | SCE_O_APPEND, 0);
			sceIoWrite(fd, "\n", 1);
			sceIoWrite(fd, taihen_config_updated_msg, sizeof(taihen_config_updated_msg) - 1);
			sceIoWrite(fd, taihen_config, sizeof(taihen_config) - 1);
			sceIoClose(fd);
		}
	}

	if (exists("ux0:app/MLCL00001/henkaku.suprx")) {
		DRAWF("Removing old henkaku.suprx\n");
		sceIoRemove("ux0:app/MLCL00001/henkaku.suprx");
	}

	if (exists("ux0:app/MLCL00001/henkaku.skprx")) {
		DRAWF("Removing old henkaku.skprx\n");
		sceIoRemove("ux0:app/MLCL00001/henkaku.skprx");
	}

	if (exists("ux0:tai/taihen.skprx")) {
		DRAWF("Removing old taihen.skprx\n");
		sceIoRemove("ux0:tai/taihen.skprx");
	}

	return 0;
}

int verify_taihen(void) {
	uint32_t crc;
	if (TAIHEN_CRC32 > 0) { // 0 skips checks
		crc = crc32_file(TAIHEN_SKPRX_FILE);
		DRAWF("taihen.skprx CRC32: 0x%08X\n", crc);
		if (crc != TAIHEN_CRC32) return -1;
		crc = crc32_file(HENKAKU_SKPRX_FILE);
		DRAWF("henkaku.skprx CRC32: 0x%08X\n", crc);
		if (crc != HENKAKU_CRC32) return -1;
		crc = crc32_file(HENKAKU_SUPRX_FILE);
		DRAWF("henkaku.suprx CRC32: 0x%08X\n", crc);
		if (crc != HENKAKU_USER_CRC32) return -1;
	}
	return 1;
}

int should_reinstall(void) {
	SceCtrlData buf;
	int ret;

	uint64_t start = sceKernelGetSystemTimeWide();
	while (1) {
		ret = sceCtrlPeekBufferPositive(0, &buf, 1);
		if (ret < 0) {
			return 0;
		}

		if (buf.buttons) {
			if (buf.buttons & (SCE_CTRL_R1 | SCE_CTRL_RTRIGGER)) {
				return 1;
			} else {
				return 0;
			}
		}

		if (sceKernelGetSystemTimeWide() - start >= (2 * 1000 * 1000)) {
			return 0;
		}

		sceKernelDelayThread(10 * 1000);
	}
}

static int load_paf() {
	unsigned ptr[0x100];
	sceClibMemset(ptr, 0, sizeof(ptr));
	ptr[0] = 0;
	ptr[1] = (unsigned)&ptr[0];
	unsigned scepaf_argp[] = {0x400000, 0xEA60, 0x40000, 0, 0};
	return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(scepaf_argp), scepaf_argp, ptr);
}

static void init_modules() {
	SceNetInitParam netInitParam;
	int ret;
	void *base;

	ret = load_paf();
	LOG("sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF): %x\n", ret);

	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
	LOG("sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL): %x\n", ret);

	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_NET);
	LOG("sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_NET): %x\n", ret);

	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_HTTP);
	LOG("sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_HTTP): %x\n", ret);

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
	int offline;
	int tries = INSTALL_ATTEMPTS;

	LOG("hello world! argc: %d\n", argc);
	syscall_id = *(uint16_t *)args;
	LOG("syscall_id: %x\n", syscall_id);
	offline = ((uint8_t *)args)[2];

	ret = sceShellUtilInitEvents(0);
	LOG("sceShellUtilInitEvents: %x\n", ret);
	ret = sceShellUtilLock(7);
	LOG("sceShellUtilLock: %x\n", ret);

	LOG("killing parent\n");
	ret = sceAppMgrDestroyOtherApp();
	LOG("sceAppMgrDestroyOtherApp: %x\n", ret);

	init_modules();

	cui_data.fg_color = 0xFFFFFFFF;

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

	if (!offline) {
		DRAWF("Press R1 now to reset HENkaku settings and reinstall molecularShell, or press any other key to continue\n");
		DRAWF("(the application will continue automatically in 2s)\n\n");

		cui_data.fg_color = 0xFFFF00FF;
		if (should_reinstall()) {
			DRAWF("Forcing reinstall of taiHEN and molecularShell, configuration will be reset\n\n");
			sceIoRemove("ux0:temp/app_work/MLCL00001/rec/first_boot.bin");
			sceIoRemove("ux0:app/MLCL00001/eboot.bin");
			sceIoRemove("ux0:app/MLCL00001/henkaku.suprx");
			sceIoRemove("ux0:app/MLCL00001/henkaku.skprx");
			sceIoRemove("ux0:tai/taihen.skprx");
			sceIoRemove(CONFIG_PATH);
			sceIoRemove(OLD_CONFIG_PATH);
			sceIoRemove(HENKAKU_SUPRX_FILE);
			sceIoRemove(HENKAKU_SKPRX_FILE);
			sceIoRemove(TAIHEN_SKPRX_FILE);
			sceIoRemove(TAIHEN_CONFIG_FILE);
			sceIoRemove(TAIHEN_RECOVERY_CONFIG_FILE);
		}
		cui_data.fg_color = 0xFFFFFFFF;
	}

	DRAWF("starting...\n");

	sceKernelDelayThread(1000 * 1000);

	LOG("am still running\n");

	int crc[3] = {0};
	const char *force_reinstall = "(if you want to force reinstall, reboot your Vita and press R1 when asked to)\n";
	while (tries-- > 0 && !offline) {
		// check if we actually need to install the package
		if (TAIHEN_CRC32 == 0 || (crc[0] = crc32_file(TAIHEN_SKPRX_FILE)) != TAIHEN_CRC32 || 
			  (crc[1] = crc32_file(HENKAKU_SUPRX_FILE)) != HENKAKU_USER_CRC32 ||
			  (crc[2] = crc32_file(HENKAKU_SKPRX_FILE)) != HENKAKU_CRC32) {
			DRAWF("taihen.skprx CRC32:%x, latest:%x\n", crc[0], TAIHEN_CRC32);
			if (crc[1]) DRAWF("henkaku.suprx CRC32:%x, latest:%x\n", crc[1], HENKAKU_USER_CRC32);
			if (crc[2]) DRAWF("henkaku.skprx CRC32:%x, latest:%x\n", crc[2], HENKAKU_CRC32);
			ret = install_taihen(PKG_URL_PREFIX);
		} else {
			DRAWF("taiHEN already installed and is the latest version\n");
			DRAWF(force_reinstall);
			ret = 0;
		}
		if (ret < 0) {
			cui_data.fg_color = 0xFF0000FF;
			DRAWF("HENkaku failed to install taiHEN: error code 0x%x, retrying (%d tries left)...\n", ret, tries);
			cui_data.fg_color = 0xFFFFFFFF;
			continue;
		}
		// check if we actually need to install the package
		if (dev_exists("ux0:data")) {
			if (!exists(CONFIG_PATH) || exists("ux0:app/MLCL00001/eboot.bin")) {
				if (VITASHELL_CRC32 == 0 || (crc[0] = crc32_file("ux0:app/MLCL00001/eboot.bin")) != VITASHELL_CRC32) {
					DRAWF("molecularShell CRC32:%x, latest:%x\n", crc[0], VITASHELL_CRC32);
					DRAWF("Getting latest version...\n");
					ret = install_pkg(PKG_URL_PREFIX);
				} else {
					DRAWF("molecularShell already installed and is the latest version\n");
					DRAWF(force_reinstall);
					ret = 0;
				}
				if (ret < 0) {
					cui_data.fg_color = 0xFF0000FF;
					DRAWF("HENkaku failed to install pkg: error code 0x%x, retrying (%d tries left)...\n", ret, tries);
					cui_data.fg_color = 0xFFFFFFFF;
					continue;
				}
			} else {
				DRAWF("molecularShell has been manually removed, skipping reinstallation\n");
				DRAWF(force_reinstall);
			}
		} else {
			DRAWF("No memory card installed, skipping installation of molecularShell\n");
		}
		// update config if needed
		if (update_taihen_config() < 0) {
			cui_data.fg_color = 0xFF0000FF;
			DRAWF("ERROR: failed to update installation, trying to reinstall (%d tries left)...\n", tries);
			cui_data.fg_color = 0xFFFFFFFF;
		}
		// verify installation
		if (verify_taihen() < 0) {
			cui_data.fg_color = 0xFF0000FF;
			DRAWF("ERROR: taiHENkaku files are corrupted, trying to reinstall (%d tries left)...\n", tries);
			cui_data.fg_color = 0xFFFFFFFF;
			sceIoRemove(TAIHEN_SKPRX_FILE);
			sceIoRemove(HENKAKU_SKPRX_FILE);
			sceIoRemove(HENKAKU_SUPRX_FILE);
		} else {
			break;
		}
	}

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
