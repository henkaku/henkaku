#include <inttypes.h>
#include "args.h"

#include "aes_key.c"

// ALL 3.60 SPECIFIC SECTIONS ARE MARKED WITH "// BEGIN 3.60"

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

typedef struct module_imports_2
{
  u16_t size; // 0x24
  u16_t version;
  u16_t flags;
  u16_t num_functions;
  u32_t reserved1;
  u32_t lib_nid;
  char     *lib_name;
  u32_t *func_nid_table;
  void     **func_entry_table;
  u32_t unk1;
  u32_t unk2;
} module_imports_2_t;

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

static inline int memcpy(void *dst, const void *src, int len) {
	char *dst1 = (char *)dst;
	const char *src1 = (const char *)src;
	while (len > 0) {
		*dst1++ = *src1++;
		len--;
	}
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

void *find_import(module_info_t *mod, uint32_t lnid, uint32_t fnid) {
	if (!mod->stub_top)
		return NULL;

	for (unsigned stub = mod->stub_top; stub < mod->stub_end;) {
		// Older fw versions use a different import format
		module_imports_2_t *imports = (module_imports_2_t *)((unsigned int)mod + 0x5C + stub - mod->stub_top);

		if (imports->size == sizeof(module_imports_2_t) && imports->lib_nid == lnid) {
			for (int j = 0; j < imports->num_functions; ++j)
				if (imports->func_nid_table[j] == fnid)
					return (void*)((u32_t *)imports->func_entry_table)[j];
		}

		stub += imports->size;
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
void (*SceCpuForDriver_19f17bd0_flush_icache)(void *addr, uint32_t size) = 0;
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
int (*sceKernelLoadStartModuleForPid)(int pid, const char *path, int args, void *argp, int flags, int *option, int *status) = 0;
int (*sceKernelLoadModuleWithoutStartForDriver)(const char *path, int flags, int *opt) = 0;
int (*sceKernelStartModuleForDriver)(int modid, int argc, void *args, int flags, void *opt, int *res) = 0;
int (*sceKernelLoadStartModuleForDriver)(const char *path, int argc, void *args, int flags) = 0;
int (*SceCpuForDriver_9CB9F0CE_flush_dcache)(uint32_t addr, int len) = 0;
int (*sceKernelDeleteThreadForKernel)(int tid) = 0;
int (*SceSblAIMgrForDriver_F4B98F66)(void) = 0;
int (*SceDebugForKernel_F857CDD6_set_crash_flag)(int) = 0;

module_info_t *modulemgr_info;
module_info_t *scenpdrm_info = 0;
unsigned modulemgr_base = 0;
unsigned modulemgr_data = 0;
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

int hook_SceSblAIMgrForDriver_D78B04A2(void)
{
	return 1;
}

int hook_SceSblAIMgrForDriver_F4B98F66(void)
{
	return 1;
}

// setup file decryption
unsigned hook_sbl_F3411881(unsigned a1, unsigned a2, unsigned a3, unsigned a4) {
	LOG("sbl_F3411881(0x%x, 0x%x, 0x%x, 0x%x)\n", a1, a2, a3, a4);

	unsigned res = hook_resume_sbl_F3411881(a1, a2, a3, a4);
	unsigned *somebuf = (unsigned*)a4;
	u64_t authid;

	if (res == 0x800f0624 || res == 0x800f0616 || res == 0x800f0024 || res == 0x800f0b3a) {
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
	// BEGIN 3.60
	u32_t *syscall_table = (u32_t*) (*((u32_t*)(modulemgr_data + 0x334)));
	// END 3.60

	for (int i = 0; i < 0x1000; ++i)
	{
		if (syscall_table[i] == addr)
		{
			SceModulemgrForKernel_0xB427025E_set_syscall(i, function);
			LOG("found syscall %x for %p\n", i, addr);
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

char old_sbl_F3411881[16];
char old_sbl_89CCDA2C[16];
char old_sbl_BC422443[16];
char old_sblai_D78B04A2[16];
char old_sblai_F4B98F66[16];

void temp_pkgpatches(void) {
	void *addr;
	LOG("inserting temporary patches\n");
	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xD78B04A2);
	LOG("sblai_D78B04A2 stub: %p\n", addr);
	DACR_OFF(
		memcpy(old_sblai_D78B04A2, addr, 16);
		INSTALL_HOOK(hook_SceSblAIMgrForDriver_D78B04A2, addr);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("hooked sblai_D78B04A2\n");
	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xF4B98F66);
	LOG("sblai_F4B98F66 stub: %p\n", addr);
	DACR_OFF(
		memcpy(old_sblai_F4B98F66, addr, 16);
		INSTALL_HOOK(hook_SceSblAIMgrForDriver_F4B98F66, addr);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("hooked sblai_F4B98F66\n");

	__asm__ volatile ("isb" ::: "memory");
}

void remove_pkgpatches(void) {
	void *addr;
	LOG("removing temporary patches\n");
	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xD78B04A2);
	LOG("sblai_D78B04A2 stub: %p\n", addr);
	DACR_OFF(
		memcpy(addr, old_sblai_D78B04A2, 16);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("unhooked sblai_D78B04A2\n");
	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xF4B98F66);
	LOG("sblai_F4B98F66 stub: %p\n", addr);
	DACR_OFF(
		memcpy(addr, old_sblai_F4B98F66, 16);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("unhooked sblai_F4B98F66\n");

	__asm__ volatile ("isb" ::: "memory");
}

void temp_sigpatches(void) {
	void *addr;
	LOG("inserting temporary patches\n");
	addr = find_import(modulemgr_info, 0x7ABF5135, 0xF3411881);
	LOG("sbl_F3411881 stub: %p\n", addr);
	DACR_OFF(
		memcpy(old_sbl_F3411881, addr, 16);
		INSTALL_HOOK(hook_sbl_F3411881, addr);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("hooked sbl_F3411881\n");
	addr = find_import(modulemgr_info, 0x7ABF5135, 0x89CCDA2C);
	LOG("sbl_89CCDA2C stub: %p\n", addr);
	DACR_OFF(memcpy(old_sbl_89CCDA2C, addr, 16));
	DACR_OFF(
		memcpy(old_sbl_89CCDA2C, addr, 16);
		INSTALL_HOOK(hook_sbl_89CCDA2C, addr);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("hooked sbl_89CCDA2C\n");
	addr = find_import(modulemgr_info, 0x7ABF5135, 0xBC422443);
	LOG("sbl_BC422443 stub: %p\n", addr);
	DACR_OFF(
		memcpy(old_sbl_BC422443, addr, 16);
		INSTALL_HOOK(hook_sbl_BC422443, addr);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("hooked sbl_BC422443\n");

	__asm__ volatile ("isb" ::: "memory");
}

void remove_sigpatches(void) {
	void *addr;
	LOG("removing temporary patches\n");
	addr = find_import(modulemgr_info, 0x7ABF5135, 0xF3411881);
	LOG("sbl_F3411881 stub: %p\n", addr);
	DACR_OFF(
		memcpy(addr, old_sbl_F3411881, 16);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("unhooked sbl_F3411881\n");
	addr = find_import(modulemgr_info, 0x7ABF5135, 0x89CCDA2C);
	LOG("sbl_89CCDA2C stub: %p\n", addr);
	DACR_OFF(
		memcpy(addr, old_sbl_89CCDA2C, 16);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("unhooked sbl_89CCDA2C\n");
	addr = find_import(modulemgr_info, 0x7ABF5135, 0xBC422443);
	LOG("sbl_BC422443 stub: %p\n", addr);
	DACR_OFF(
		memcpy(addr, old_sbl_BC422443, 16);
		SceCpuForDriver_19f17bd0_flush_icache((uint32_t)addr & ~0x1F, 0x20);
		SceCpuForDriver_9CB9F0CE_flush_dcache((uint32_t)addr & ~0x1F, 0x20);
	);
	LOG("unhooked sbl_BC422443\n");

	__asm__ volatile ("isb" ::: "memory");
}

void thread_main(void) {
	unsigned ret;

	// halt on crash
#if !RELEASE
	LOG("disabling auto-reboot\n");
	SceDebugForKernel_F857CDD6_set_crash_flag(0);
#endif

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
}

int takeover_web_browser() {
	unsigned ret;
	unsigned base = 0;
#ifdef OFFLINE
	base = SceEmailEngine_base + 0xE6F70;
#else
	base = SceWebBrowser_base + 0xC6200;
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
	temp_pkgpatches();

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
	sceKernelDeleteThreadForKernel(thread);

	// undo patches
	remove_pkgpatches();
	patch_syscall((u32_t)sceDisplaySetFrameBufInternalPatched, sceDisplaySetFrameBufInternal);
	patch_syscall((u32_t)sceDisplayGetFrameBufInternalPatched, sceDisplayGetFrameBufInternal);
	patch_syscall((u32_t)hook_sharedfb_update_begin, (void *)sharedfb_update_begin);
	patch_syscall((u32_t)hook_sharedfb_update_process, (void *)sharedfb_update_process);
	patch_syscall((u32_t)hook_sharedfb_update_end, (void *)sharedfb_update_end);

	return status;
}

int load_taihen(int args, void *argp) {
	unsigned opt, taiid, modid, ret, result;
	SceModInfo info;
	module_info_t *taihen_info;
	int (*taiLoadPluginsForTitleForKernel)(int pid, const char *titleid, int flags);
	// load taiHEN
	temp_sigpatches();
	opt = 4;
	taiid = sceKernelLoadModuleWithoutStartForDriver("ux0:tai/taihen.skprx", 0, &opt);
	LOG("LoadTaiHEN: 0x%08X\n", taiid);
	remove_sigpatches();
	LOG("Removed temp patches\n");
	ret = sceKernelStartModuleForDriver(taiid, 0, NULL, 0, NULL, &result);
	result = -1;
	LOG("StartTaiHEN: 0x%08X, 0x%08X\n", ret, result);
	if (result < 0) {
		ret = result;
		goto end;
	}

	// load henkaku kernel
	modid = sceKernelLoadModuleWithoutStartForDriver("ux0:app/MLCL00001/henkaku.skprx", 0, &opt);
	LOG("LoadHENKaku kernel: 0x%08X\n", modid);
	result = -1;
	ret = sceKernelStartModuleForDriver(modid, 4, &shell_pid, 0, NULL, &result);
	LOG("StartHENkaku kernel: 0x%08X, 0x%08X\n", ret, result);
	if (result < 0) {
		ret = result;
		goto end;
	}

end:
	return ret;
}

void resolve_imports(unsigned sysmem_base) {
	unsigned ret;

	module_info_t *sysmem_info = find_modinfo(sysmem_base, "SceSysmem");

	// BEGIN 3.60 specific offsets here, used to find Modulemgr from just sysmem base
	LOG("sysmem base: 0x%08x\n", sysmem_base);
	void *sysmem_data = (void*)(*(u32_t*)((u32_t)(sysmem_base) + 0x26a68) - 0xA0);
	LOG("sysmem data base: 0x%08x\n", sysmem_data);
	DACR_OFF(modulemgr_base = (*(u32_t*)((u32_t)(sysmem_data) + 0x438c) - 0x40););
	LOG("modulemgr base: 0x%08x\n", modulemgr_base);
	// END 3.60 specific offsets

	DACR_OFF(modulemgr_info = find_modinfo((u32_t)modulemgr_base, "SceKernelModulemgr"));
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
			DACR_OFF(scenpdrm_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceNpDrm"));
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
	LOG("scenpdrm_info: 0x%08x | scepower_code: 0x%08x\n", scenpdrm_info, scepower_code);

	LOG("Fixup: unlock SceNetPs global mutex ");
	// BEGIN 3.60
	int (*sce_psnet_bnet_mutex_unlock)() = (void*)(scenet_code + 0x2a098|1);
	void *mutex = (void*)((u32_t)scenet_data + 0x850);
	// END 3.60
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
		SceCpuForDriver_19f17bd0_flush_icache = find_export(sysmem_info, 0x19f17bd0);
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
		sceKernelLoadModuleWithoutStartForDriver = find_export(modulemgr_info, 0x86D8D634);
		sceKernelStartModuleForDriver = find_export(modulemgr_info, 0x0675B682);
		sceKernelLoadStartModuleForDriver = find_export(modulemgr_info, 0x189BFBBB);
		sceKernelLoadStartModuleForPid = find_export(modulemgr_info, 0x9d953c22);
		SceCpuForDriver_9CB9F0CE_flush_dcache = find_export(sysmem_info, 0x9CB9F0CE);
		sceKernelDeleteThreadForKernel = find_export(threadmgr_info, 0xac834f3f);
		SceDebugForKernel_F857CDD6_set_crash_flag = find_export(sysmem_info, 0xF857CDD6);
	);
}

void __attribute__ ((section (".text.start"))) payload(uint32_t sysmem_addr) {
	// find sysmem base, etc
	uint32_t sysmem_base = sysmem_addr;
	int ret;
	// BEGIN 3.60
	void (*debug_print_local)(char *s, ...) = (void*)(sysmem_base + 0x1A155);
	// END 3.60

	DACR_OFF(
		debug_print = debug_print_local;
	);

	LOG("+++ Entered kernel payload +++\n");
	LOG("payload=0x%x, sp=0x%x\n", payload, &ret);

	#ifdef OFFLINE
	LOG("(offline version)\n");
	#endif

	resolve_imports(sysmem_base);
	thread_main();
	ret = takeover_web_browser();
	if (ret == 0) {
		int tid;
		int res;
		tid = sceKernelCreateThreadForKernel("load_taihen", load_taihen, 64, 0x4000, 0, 0x10000, 0);
		LOG("load_taihen tid: %x\n", tid);
		ret = sceKernelStartThread_089(tid, 0, NULL);
		LOG("start thread: %x\n", ret);
		ret = sceKernelWaitThreadEndForKernel(tid, &res, NULL);
		LOG("thread returned: %x, %x\n", ret, res);
		sceKernelDeleteThreadForKernel(tid);
	}
	else
		LOG("skipping taihen loading\n");

	LOG("Kill current thread =>");
	LOG("sceKernelExitDeleteThread at 0x%08x\n", sceKernelExitDeleteThread);
	ret = sceKernelExitDeleteThread();
	LOG("??? ret = 0x%08x\n", ret);
	LOG("something broke...");

	while(1) {}
}
