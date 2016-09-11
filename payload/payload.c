#include <inttypes.h>
#include "args.h"

#include "aes_key.c"
#include "../build/version.c"

#if RELEASE
#define LOG(fmt, ...)
#else
#define LOG debug_print
#endif

#define NULL ((void*)0)

#define DACR_OFF(stmt)                 \
do {                                   \
	unsigned prev_dacr;                \
	__asm__ volatile(                  \
		"mrc p15, 0, %0, c3, c0, 0 \n" \
		: "=r" (prev_dacr)             \
	);                                 \
	__asm__ volatile(                  \
		"mcr p15, 0, %0, c3, c0, 0 \n" \
		: : "r" (0xFFFF0000)           \
	);                                 \
	stmt;                              \
	__asm__ volatile(                  \
		"mcr p15, 0, %0, c3, c0, 0 \n" \
		: : "r" (prev_dacr)            \
	);                                 \
} while (0)

#define INSTALL_HOOK(func, addr) \
do {                                               \
	unsigned *target;                              \
	target = (unsigned*)(addr);                    \
	*target++ = 0xE59FF000; /* ldr pc, [pc, #0] */ \
	*target++; /* doesn't matter */                \
	*target = (unsigned)func;                      \
} while (0)

#define INSTALL_HOOK_THUMB(func, addr) \
do {                                                \
	unsigned *target;                                 \
	target = (unsigned*)(addr);                       \
	*target++ = 0xC004F8DF; /* ldr.w	ip, [pc, #4] */ \
	*target++ = 0xBF004760; /* bx ip; nop */          \
	*target = (unsigned)func;                         \
} while (0)

#define INSTALL_RET(addr, ret) \
do {                                            \
	unsigned *target;                             \
	target = (unsigned*)(addr);                   \
	*target++ = 0xe3a00000 | ret; /* mov r0, #ret */ \
	*target = 0xe12fff1e; /* bx lr */             \
} while (0)

#define INSTALL_RET_THUMB(addr, ret) \
do {                                            \
	unsigned *target;                             \
	target = (unsigned*)(addr);                   \
	*target = 0x47702000 | ret; /* movs r0, #ret; bx lr */ \
} while (0)

typedef uint64_t u64_t;
typedef uint32_t u32_t;
typedef uint16_t u16_t;
typedef uint8_t u8_t;

typedef struct segment_info
{
	int           size;   // this structure size (0x18)
	int           perms;  // probably rwx in low bits
	void            *vaddr; // address in memory
	int           memsz;  // size in memory
	int           flags;  // meanig unknown
	int           res;    // unused?
} segment_info_t;

typedef struct SceModInfo {
	int size;           //0
	int UID;            //4
	int mod_attr;       //8
	char name[0x1C];        //0xC
	u32_t unk0;         //0x28
	void *module_start;     //0x2C addr0
	void *module_init;      //0x30 addr1
	void *module_stop;      //0x34 addr2
	void *exidx_start;      //0x38 addr3
	void *exidx_end;        //0x3C addr4
	void *addr5;        //0x40 addr5
	void *addr6;        //0x44 addr6
	void *module_top;       //0x48 addr7
	void *addr8;        //0x4C addr8
	void *addr9;        //0x50 addr9
	char filepath[0x100];   //0x54
	segment_info_t  segments[4]; //0x58
	u32_t unk2;         //0x1B4
} SceModInfo; //0x1B8

#define MOD_LIST_SIZE 0x80

typedef struct module_imports // thanks roxfan
{
	u16_t   size;               // size of this structure; 0x34 for Vita 1.x
	u16_t   lib_version;        //
	u16_t   attribute;          //
	u16_t   num_functions;      // number of imported functions
	u16_t   num_vars;           // number of imported variables
	u16_t   num_tls_vars;       // number of imported TLS variables
	u32_t   reserved1;          // ?
	u32_t   module_nid;         // NID of the module to link to
	char    *lib_name;          // name of module
	u32_t   reserved2;          // ?
	u32_t   *func_nid_table;    // array of function NIDs (numFuncs)
	void    **func_entry_table; // parallel array of pointers to stubs; they're patched by the loader to jump to the final code
	u32_t   *var_nid_table;     // NIDs of the imported variables (numVars)
	void    **var_entry_table;  // array of pointers to "ref tables" for each variable
	u32_t   *tls_nid_table;     // NIDs of the imported TLS variables (numTlsVars)
	void    **tls_entry_table;  // array of pointers to ???
} module_imports_t;

typedef struct module_exports // thanks roxfan
{
	u16_t   size;           // size of this structure; 0x20 for Vita 1.x
	u8_t    lib_version[2]; //
	u16_t   attribute;      // ?
	u16_t   num_functions;  // number of exported functions
	u32_t   num_vars;       // number of exported variables
	u32_t   num_tls_vars;   // number of exported TLS variables?  <-- pretty sure wrong // yifanlu
	u32_t   module_nid;     // NID of this specific export list; one PRX can export several names
	char    *lib_name;      // name of the export module
	u32_t   *nid_table;     // array of 32-bit NIDs for the exports, first functions then vars
	void    **entry_table;  // array of pointers to exported functions and then variables
} module_exports_t;

typedef struct module_info // thanks roxfan
{
	u16_t   modattribute;  // ??
	u16_t   modversion;    // always 1,1?
	char    modname[27];   ///< Name of the module
	u8_t    type;          // 6 = user-mode prx?
	void    *gp_value;     // always 0 on ARM
	int   ent_top;       // beginning of the export list (sceModuleExports array)
	int   ent_end;       // end of same
	int   stub_top;      // beginning of the import list (sceModuleStubInfo array)
	int   stub_end;      // end of same
	u32_t   module_nid;    // ID of the PRX? seems to be unused
	int   field_38;      // unused in samples
	int   field_3C;      // I suspect these may contain TLS info
	int   field_40;      //
	int   mod_start;     // 44 module start function; can be 0 or -1; also present in exports
	int   mod_stop;      // 48 module stop function
	int   exidx_start;   // 4c ARM EABI style exception tables
	int   exidx_end;     // 50
	int   extab_start;   // 54
	int   extab_end;     // 58
} module_info_t; // 5c?


int strcmp(const char *s1, const char *s2) {
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}

module_info_t * find_modinfo(uint32_t start_addr, const char *needle) {
	start_addr &= ~0xF;
	start_addr += 4;

	while (1) {
		if (strcmp((const char *)start_addr, needle) == 0)
			 return (module_info_t *)(start_addr - 0x4);

		start_addr += 0x10;
	}
}

void *find_export(module_info_t *mod, uint32_t nid) {
	if (!mod->ent_top)
		return NULL;

	for (unsigned ent = mod->ent_top; ent < mod->ent_end;) {
		module_exports_t *exports = (module_exports_t *)((unsigned int)mod + 0x5C + ent - mod->ent_top);

		for (int j = 0; j < exports->num_functions; ++j)
			if (exports->nid_table[j] == nid)
				return (void*)((u32_t *)exports->entry_table)[j];

		ent += exports->size;
	}

	return NULL;
}

void (*debug_print)(char *s, ...) = 0;

unsigned g_homebrew_decrypt = 0;
int (*hook_resume_sbl_F3411881)() = 0;
int (*hook_resume_sbl_89CCDA2C)() = 0;
int (*hook_resume_sbl_BC422443)() = 0;

int (*sceKernelGetProcessId)() = 0;
int (*SceProcessmgrForDriver_0AFF3EAE)() = 0;
int (*unrestricted_memcpy_for_pid)(int pid, void *dst, void *src, unsigned len) = 0;
int (*sceKernelMemcpyUserToKernelForPid)(int pid, void *dst, void *src, unsigned len) = 0;
int (*sceKernelCreateThreadForPid)() = 0;
int (*sceKernelStartThread_089)() = 0;
int (*sceKernelAllocMemBlockForKernel)() = 0;
int (*sceKernelGetMemBlockBaseForKernel)() = 0;
void (*SceCpuForDriver_9CB9F0CE_flush_icache)(void *addr, uint32_t size) = 0;
int (*sceKernelCreateThreadForKernel)() = 0;
int (*sceKernelExitDeleteThread)() = 0;
int (*sceKernelGetModuleListForKernel)() = 0;
int (*sceKernelGetModuleInfoForKernel)() = 0;
void (*SceModulemgrForKernel_0xB427025E_set_syscall)(u32_t num, void *function) = 0;
int (*sceDisplaySetFrameBufInternal)(int r0, int r1, int r2, int r3) = 0;
int (*sceDisplayGetFrameBufInternal)(int r0, int r1, int r2, int r3) = 0;
int (*sceKernelDelayThread)(int ms) = 0;
int (* SceThreadmgrForKernel_0xEA7B8AEF_get_thread_list)(int pid, void *buf, int bufsize, int *count) = 0;
int (* sceKernelChangeThreadCpuAffinityMask)(int thid, int mask) = 0;
int (* sceKernelChangeThreadPriority)(int thid, int priority) = 0;
int (*sceKernelWaitThreadEndForKernel)(int thid, int *r1, int *r2) = 0;
int (*sblAimgrIsCEX)(void) = 0;
void (*aes_setkey)(void *ctx, uint32_t blocksize, uint32_t keysize, void *key) = 0;
void (*aes_encrypt)(void *ctx, void *src, void *dst) = 0;

unsigned modulemgr_base = 0;
unsigned modulemgr_data = 0;
unsigned scenpdrm_code = 0;
int pid = 0, ppid = 0, shell_pid = 0;
unsigned SceWebBrowser_base = 0;
unsigned SceLibKernel_base = 0;
unsigned SceEmailEngine_base = 0;
unsigned SceShell_base = 0, SceShell_size = 0;
u32_t sharedfb_update_begin = 0, sharedfb_update_process = 0, sharedfb_update_end = 0;
unsigned scepower_code = 0;

char pkg_url_prefix[256] __attribute__((aligned(16))) __attribute__ ((section (".pkgurl"))) = "PKG_URL_PREFIX_PLACEHOLDER";

int sceDisplaySetFrameBufInternalPatched(int r0, int r1, int r2, int r3)
{
	return 0;
}

int sceDisplayGetFrameBufInternalPatched(int r0, int r1, int r2, int r3)
{
	return 0;
}

int hook_sharedfb_update_begin(int r0, int r1, int r2, int r3)
{
	return 0;
}

int hook_sharedfb_update_process(int r0, int r1, int r2, int r3)
{
	return 0;
}

int hook_sharedfb_update_end(int r0, int r1, int r2, int r3)
{
	return 0;
}

// setup file decryption
unsigned hook_sbl_F3411881(unsigned a1, unsigned a2, unsigned a3, unsigned a4) {
	LOG("sbl_F3411881(0x%x, 0x%x, 0x%x, 0x%x)\n", a1, a2, a3, a4);

	unsigned res = hook_resume_sbl_F3411881(a1, a2, a3, a4);
	unsigned *somebuf = (unsigned*)a4;
	u64_t authid;

	if (res == 0x800f0624 || res == 0x800f0616 || res == 0x800f0024) {
		DACR_OFF(
			g_homebrew_decrypt = 1;
		);

		// patch somebuf so our module actually runs
		authid = *(u64_t *)(a2+0x80);
		LOG("authid: 0x%llx\n", authid);
		if ((authid & 0xFFFFFFFFFFFFFFFDLL) == 0x2F00000000000001LL) {
			somebuf[42] = 0x40;
		} else {
			somebuf[42] = 0x20;
		}

		return 0;
	} else {
		DACR_OFF(
			g_homebrew_decrypt = 0;
		);
	}
	return res;
}

// setup output buffer
unsigned hook_sbl_89CCDA2C(unsigned a1, unsigned a2) {
	LOG("sbl_89CCDA2C(0x%x, 0x%x) hb=0x%x\n", a1, a2, g_homebrew_decrypt);
	if (g_homebrew_decrypt == 1)
		return 1;
	return hook_resume_sbl_89CCDA2C(a1, a2);
}

// decrypt
unsigned hook_sbl_BC422443(unsigned a1, unsigned a2, unsigned a3) {
	LOG("sbl_BC422443(0x%x, 0x%x, 0x%x) hb=0x%x\n", a1, a2, a3, g_homebrew_decrypt);
	if (g_homebrew_decrypt == 1)
		return 0;
	return hook_resume_sbl_BC422443(a1, a2, a3);
}

void print_buffer(unsigned *buffer) {
	for (int i = 0; i < 0x100; ++i)
		LOG("0x%x: 0x%x\n", i, buffer[i]);
}

void patch_syscall(u32_t addr, void *function)
{
	u32_t *syscall_table = (u32_t*) (*((u32_t*)(modulemgr_data + 0x334)));

	for (int i = 0; i < 0x1000; ++i)
	{
		if (syscall_table[i] == addr)
		{
			SceModulemgrForKernel_0xB427025E_set_syscall(i, function);
		}
	}
}

// this is user shellcode
#ifdef OFFLINE
#define build_offline_user_bin build_user_bin
#define build_offline_user_bin_len build_user_bin_len
#include "../build/offline/user.h"
#else
#include "../build/user.h"
#endif

void thread_main(unsigned sysmem_base) {
	unsigned ret;

	// homebrew enable
	uint32_t *patch;
	DACR_OFF(
		INSTALL_HOOK(hook_sbl_F3411881, (char*)modulemgr_base + 0xb68c); // 3.60
		INSTALL_HOOK(hook_sbl_89CCDA2C, (char*)modulemgr_base + 0xb64c); // 3.60
		INSTALL_HOOK(hook_sbl_BC422443, (char*)modulemgr_base + 0xb67c); // 3.60

		// patch vm code alloc checks
		INSTALL_RET_THUMB((char *)sysmem_base + 0x1fa8c, 1); // 3.60
		INSTALL_RET_THUMB((char *)sysmem_base + 0x8c00, 0); // 3.60

		// patch error code 0x80870003 C0-9249-4
		patch = (void*)(scenpdrm_code + 0x8068); // 3.60
		*patch = 0x2500; // mov r5, 0
		patch = (void*)(scenpdrm_code + 0x9994); // 3.60
		*patch = 0x2600; // mov r6, 0

		patch = (void*)(scenpdrm_code + 0x6A38);
		*patch = 0x47702001; // always return 1 in install_allowed


	);
	SceCpuForDriver_9CB9F0CE_flush_icache((void*)scenpdrm_code, 0x12000); // and npdrm patches
	// end homebrew enable

	// ScePower unlock for safe homebrew
	DACR_OFF(
		INSTALL_RET((char *)scepower_code + 0x45f0, 1); // 3.60
	);
	SceCpuForDriver_9CB9F0CE_flush_icache((void*)scepower_code, 0xa600); //
	// end ScePower patch

	// patch version information

	*(unsigned *)(modulemgr_data + 0x34) = 1;
	*(unsigned *)(modulemgr_data + 0x2d0) = 0x28; // size
	*(unsigned *)(modulemgr_data + 0x2d4) = 0x30362e33; // "3.60"
	*(unsigned *)(modulemgr_data + 0x2d8) = 0xa4e52829; // ")(.."
	*(unsigned *)(modulemgr_data + 0x2dc) = 0xa99de989; // "...."
	*(unsigned *)(modulemgr_data + 0x2e0) = 0x0000002d | ((u32_t)HEN_VERSION << 8); // "-V\0"
	*(unsigned *)(modulemgr_data + 0x2f0) = 0x3610000; // version

	/*
	// older patch sets is_cex function to return 0
	DACR_OFF(
		INSTALL_RET_THUMB((char *)vshbridge_base + 0x16f4, 0);
	);
	SceCpuForDriver_9CB9F0CE_flush_icache((void*)vshbridge_base, 0x9A00);
	*/

	// end patch version information

	// takeover the web browser or email if offline

	unsigned data[0xE8/4];
	data[0] = sizeof(data);
	SceProcessmgrForDriver_0AFF3EAE(pid, data);
	DACR_OFF(ppid = data[5];);
	LOG("Target PID: 0x%x\n", ppid);
	SceProcessmgrForDriver_0AFF3EAE(ppid, data);
	DACR_OFF(shell_pid = data[5];);
	LOG("Shell PID: 0x%x\n", shell_pid);

	int *modlist[MOD_LIST_SIZE];
	int modlist_records;
	int res;
	SceModInfo info;

	modlist_records = MOD_LIST_SIZE;
	ret = sceKernelGetModuleListForKernel(ppid, 0x7FFFFFFF, 1, modlist, &modlist_records);
	LOG("sceKernelGetModuleList() returned 0x%x\n", ret);

	for (int i = 0; i < modlist_records; ++i) {
		info.size = sizeof(info);
		ret = sceKernelGetModuleInfoForKernel(ppid, modlist[i], &info);
		unsigned addr = (unsigned)info.segments[0].vaddr;
		if (strcmp(info.name, "SceWebBrowser") == 0)
			DACR_OFF(SceWebBrowser_base = addr);
		else if (strcmp(info.name, "SceEmailEngine") == 0)
			DACR_OFF(SceEmailEngine_base = addr);
		else if (strcmp(info.name, "SceLibKernel") == 0)
			DACR_OFF(SceLibKernel_base = addr);
	}

	modlist_records = MOD_LIST_SIZE;
	ret = sceKernelGetModuleListForKernel(shell_pid, 0x7FFFFFFF, 1, modlist, &modlist_records);
	LOG("sceKernelGetModuleList() returned 0x%x\n", ret);

	for (int i = 0; i < modlist_records; ++i) {
		info.size = sizeof(info);
		ret = sceKernelGetModuleInfoForKernel(shell_pid, modlist[i], &info);
		if (strcmp(info.name, "SceShell") == 0)
			DACR_OFF(
				SceShell_base = (unsigned)info.segments[0].vaddr;
				SceShell_size = (unsigned)info.segments[0].memsz;
			);
	}
	LOG("Found SceShell at 0x%x of size 0x%x\n", SceShell_base, SceShell_size);
}

void patch_shell() {
	static unsigned char update_check_patch[] = {
	  0x00, 0x20, 0x10, 0x60, 0x70, 0x47, 0x00, 0xbf
	};

	if (sblAimgrIsCEX() == 1) {
		LOG("We are running on retail, patching update url\n");
		unrestricted_memcpy_for_pid(shell_pid, SceShell_base+0x363de8, update_check_patch, (sizeof(update_check_patch) + 0x10) & ~0xF);
	} else {
		LOG("We are running on debug, skipping shell patches\n");
	}
}

void takeover_web_browser() {
	unsigned ret;
	unsigned base = 0;
#ifdef OFFLINE
	base = SceEmailEngine_base + 0xE6F70;
#else
	base = SceWebBrowser_base + 0xC6200;
#endif
#if 0
	unsigned popt[0x58/4];
	for (int i = 0; i < 0x58/4; ++i)
		popt[i] = 0;
	popt[0/4] = 0x58;
	popt[0x8/4] = 088x50480088;
	popt[0x18/4] = 0x1000;
	popt[0x24/4] = ppid;
	ret = sceKernelAllocMemBlockForKernel("", 0xC20D050, 0x4000, popt);
	LOG("alloc memblock ret = 0x%x\n", ret);
	if (ret & 0x80000000)
		return;
	ret = sceKernelGetMemBlockBaseForKernel(ret, &base);
	LOG("getbase ret = 0x%x base = 0x%x\n", ret, base);
#endif

	int thidlist[MOD_LIST_SIZE];
	int thidlist_records = 0;
	SceThreadmgrForKernel_0xEA7B8AEF_get_thread_list(ppid, thidlist, sizeof(thidlist), &thidlist_records);

	LOG("got %d threads\n", thidlist_records);

	for (int i = 0; i < thidlist_records; ++i)
	{
		sceKernelChangeThreadCpuAffinityMask(thidlist[i], 0x40000000);
		sceKernelChangeThreadPriority(thidlist[i], 255);
	}

	// patch
	LOG("patching syscalls\n");
	patch_syscall(sharedfb_update_begin, hook_sharedfb_update_begin);
	patch_syscall(sharedfb_update_process, hook_sharedfb_update_process);
	patch_syscall(sharedfb_update_end, hook_sharedfb_update_end);
	patch_syscall((u32_t)sceDisplaySetFrameBufInternal, sceDisplaySetFrameBufInternalPatched);
	patch_syscall((u32_t)sceDisplayGetFrameBufInternal, sceDisplayGetFrameBufInternalPatched);

	// inject the code
	LOG("injecting code to pid 0x%x at 0x%x\n", ppid, base);
	unrestricted_memcpy_for_pid(ppid, (void*)base, build_user_bin, (build_user_bin_len + 0x10) & ~0xF);
	LOG("code injected\n");

	int thread = sceKernelCreateThreadForPid(ppid, "", base|1, 64, 0x4000, 0x800000, 0, 0);
	LOG("create thread 0x%x\n", thread);

	// "encrypt" the pkg info url (since it was decrypted before)

	struct args args;
	char aes_ctx[0x400];
	args.libbase = SceLibKernel_base;

	aes_setkey(aes_ctx, 128, 128, aes_key);
	for (uint32_t i = 0; i < 256; i += 0x10)
	{
		aes_encrypt(aes_ctx, &pkg_url_prefix[i], &args.pkg_url_prefix[i]);
	}

	ret = sceKernelStartThread_089(thread, sizeof(args), &args);
	LOG("sceKernelStartThread_089 ret 0x%x\n", ret);

	sceKernelDelayThread(5*1000*1000);

	int status = 0;
	sceKernelWaitThreadEndForKernel(thread, &status, NULL);

	// undo patches
	patch_syscall((u32_t)sceDisplaySetFrameBufInternalPatched, sceDisplaySetFrameBufInternal);
	patch_syscall((u32_t)sceDisplayGetFrameBufInternalPatched, sceDisplayGetFrameBufInternal);
	patch_syscall((u32_t)hook_sharedfb_update_begin, (void *)sharedfb_update_begin);
	patch_syscall((u32_t)hook_sharedfb_update_process, (void *)sharedfb_update_process);
	patch_syscall((u32_t)hook_sharedfb_update_end, (void *)sharedfb_update_end);
}

void resolve_imports(unsigned sysmem_base) {
	unsigned ret;

	module_info_t *sysmem_info = find_modinfo(sysmem_base, "SceSysmem");

	// 3.60-specific offsets here, used to find Modulemgr from just sysmem base
	LOG("sysmem base: 0x%08x\n", sysmem_base);
	void *sysmem_data = (void*)(*(u32_t*)((u32_t)(sysmem_base) + 0x26a68) - 0xA0);
	LOG("sysmem data base: 0x%08x\n", sysmem_data);
	DACR_OFF(modulemgr_base = (*(u32_t*)((u32_t)(sysmem_data) + 0x438c) - 0x40););
	LOG("modulemgr base: 0x%08x\n", modulemgr_base);
	// end of 3.60-specific offsets

	module_info_t *modulemgr_info = find_modinfo((u32_t)modulemgr_base, "SceKernelModulemgr");
	LOG("modulemgr modinfo: 0x%08x\n", modulemgr_info);

	DACR_OFF(
		sceKernelGetModuleListForKernel = find_export(modulemgr_info, 0x97CF7B4E);
		sceKernelGetModuleInfoForKernel = find_export(modulemgr_info, 0xD269F915);
	);
	LOG("sceKernelGetModuleListForKernel: %08x\n", sceKernelGetModuleListForKernel);
	LOG("sceKernelGetModuleInfoForKernel: %08x\n", sceKernelGetModuleInfoForKernel);

	int *modlist[MOD_LIST_SIZE];
	int modlist_records;
	int res;
	SceModInfo info;

	modlist_records = MOD_LIST_SIZE;
	ret = sceKernelGetModuleListForKernel(0x10005, 0x7FFFFFFF, 1, modlist, &modlist_records);
	LOG("sceKernelGetModuleList() returned 0x%x\n", ret);
	LOG("modlist_records: %d\n", modlist_records);
		module_info_t *threadmgr_info = 0, *sblauthmgr_info = 0, *processmgr_info = 0, *display_info = 0, *appmgr_info = 0;
	u32_t scenet_code = 0, scenet_data = 0;
	for (int i = 0; i < modlist_records; ++i) {
		info.size = sizeof(info);
		ret = sceKernelGetModuleInfoForKernel(0x10005, modlist[i], &info);
		if (strcmp(info.name, "SceKernelThreadMgr") == 0)
			threadmgr_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceKernelThreadMgr");
		if (strcmp(info.name, "SceSblAuthMgr") == 0)
			sblauthmgr_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceSblAuthMgr");
		if (strcmp(info.name, "SceNpDrm") == 0)
			DACR_OFF(scenpdrm_code = (u32_t)info.segments[0].vaddr;);
		if (strcmp(info.name, "SceNetPs") == 0) {
			scenet_code = (u32_t)info.segments[0].vaddr;
			scenet_data = (u32_t)info.segments[1].vaddr;
		}
		if (strcmp(info.name, "SceKernelModulemgr") == 0) {
			DACR_OFF(modulemgr_data = (u32_t)info.segments[1].vaddr;);
		}
		if (strcmp(info.name, "SceDisplay") == 0) {
			display_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceDisplay");
		}
		if (strcmp(info.name, "SceAppMgr") == 0) {
			appmgr_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceAppMgr");
		}
		if (strcmp(info.name, "SceProcessmgr") == 0) {
			processmgr_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceProcessmgr");
		}
		if (strcmp(info.name, "ScePower") == 0) {
			DACR_OFF(scepower_code = (u32_t)info.segments[0].vaddr);
		}
	}

	LOG("threadmgr_info: 0x%08x | sblauthmgr_info: 0x%08x | scenet_code: 0x%08x | scenet_data: 0x%08x\n", threadmgr_info, sblauthmgr_info, scenet_code, scenet_data);
	LOG("scenpdrm_code: 0x%08x | scepower_code: 0x%08x\n", scenpdrm_code, scepower_code);

	LOG("Fixup: unlock SceNetPs global mutex ");
	// 3.60
	int (*sce_psnet_bnet_mutex_unlock)() = (void*)(scenet_code + 0x2a098|1);
	void *mutex = (void*)((u32_t)scenet_data + 0x850);
	// end
	ret = sce_psnet_bnet_mutex_unlock(mutex);
	LOG("=> ret = 0x%08x\n", ret);

	DACR_OFF(
		hook_resume_sbl_F3411881 = find_export(sblauthmgr_info, 0xF3411881);
		hook_resume_sbl_89CCDA2C = find_export(sblauthmgr_info, 0x89CCDA2C);
		hook_resume_sbl_BC422443 = find_export(sblauthmgr_info, 0xBC422443);

		sceKernelGetProcessId = find_export(threadmgr_info, 0x9DCB4B7A);
		pid = sceKernelGetProcessId();
		SceProcessmgrForDriver_0AFF3EAE = find_export(processmgr_info, 0x0AFF3EAE);
		unrestricted_memcpy_for_pid = find_export(sysmem_info, 0x30931572);
		sceKernelMemcpyUserToKernelForPid = find_export(sysmem_info, 0x605275F8);
		sceKernelCreateThreadForPid = find_export(threadmgr_info, 0xC8E57BB4);
		sceKernelStartThread_089 = find_export(threadmgr_info, 0x21F5419B);
		sceKernelAllocMemBlockForKernel = find_export(sysmem_info, 0xC94850C9);
		sceKernelGetMemBlockBaseForKernel = find_export(sysmem_info, 0xA841EDDA);
		SceCpuForDriver_9CB9F0CE_flush_icache = find_export(sysmem_info, 0x9CB9F0CE);
		sceKernelCreateThreadForKernel = find_export(threadmgr_info, 0xC6674E7D);
		sceKernelExitDeleteThread = find_export(threadmgr_info, 0x1D17DECF);
		SceModulemgrForKernel_0xB427025E_set_syscall = find_export(modulemgr_info, 0xB427025E);
		sceDisplayGetFrameBufInternal = find_export(display_info, 0x86a8e436);
		sceDisplaySetFrameBufInternal = find_export(display_info, 0x7a8cb78e);
		sceKernelDelayThread = find_export(threadmgr_info, 0x4b675d05);
		SceThreadmgrForKernel_0xEA7B8AEF_get_thread_list = find_export(threadmgr_info, 0xEA7B8AEF);
		sceKernelChangeThreadCpuAffinityMask = find_export(threadmgr_info, 0x15129174);
		sceKernelChangeThreadPriority = find_export(threadmgr_info, 0xbd0139f2);
		sceKernelWaitThreadEndForKernel = find_export(threadmgr_info, 0x3E20216F);
		sharedfb_update_begin = (u32_t)find_export(appmgr_info, 0xf9754ad9);
		sharedfb_update_process = (u32_t)find_export(appmgr_info, 0x3889acf8);
		sharedfb_update_end = (u32_t)find_export(appmgr_info, 0x565a9ab6);
		aes_setkey = find_export(sysmem_info, 0xf12b6451);
		aes_encrypt = find_export(sysmem_info, 0xC2A61770);
		sblAimgrIsCEX = find_export(sysmem_info, 0xD78B04A2);
		);
}

void __attribute__ ((section (".text.start"))) payload(uint32_t sysmem_addr) {
	// find sysmem base, etc
	uint32_t sysmem_base = sysmem_addr;
	uint32_t ret;
	void (*debug_print_local)(char *s, ...) = (void*)(sysmem_base + 0x1A155); // 3.60

	DACR_OFF(
		debug_print = debug_print_local;
	);

	LOG("+++ Entered kernel payload +++\n");
	LOG("sp=0x%x\n", &ret);

	#ifdef OFFLINE
	LOG("(offline version)\n");
	#endif

	resolve_imports(sysmem_base);
	thread_main(sysmem_base);
	patch_shell();
	takeover_web_browser();

	LOG("Kill current thread =>");
	LOG("sceKernelExitDeleteThread at 0x%08x\n", sceKernelExitDeleteThread);
	ret = sceKernelExitDeleteThread();
	LOG("??? ret = 0x%08x\n", ret);
	LOG("something broke...");

	while(1) {}
}
