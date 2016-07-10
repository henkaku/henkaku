#include <inttypes.h>

#define DEBUG 1

#if DEBUG
#define LOG debug_print
#else
#define LOG(fmt, ...)
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

unsigned modulemgr_base = 0;
unsigned scesblacmgr_code = 0;
unsigned scenpdrm_code = 0;
int pid = 0, ppid = 0;
unsigned SceWebBrowser_base = 0;
unsigned SceLibKernel_base = 0;
unsigned SceDriverUser_base = 0;

// setup file decryption
unsigned hook_sbl_F3411881(unsigned a1, unsigned a2, unsigned a3, unsigned a4) {
	LOG("sbl_F3411881(0x%x, 0x%x, 0x%x, 0x%x)\n", a1, a2, a3, a4);

	unsigned res = hook_resume_sbl_F3411881(a1, a2, a3, a4);
	unsigned *somebuf = (unsigned*)a4;

	if (res == 0x800f0624 || res == 0x800f0616 || res == 0x800f0024) {
		DACR_OFF(
			g_homebrew_decrypt = 1;
		);

		// patch somebuf so our module actually runs
		somebuf[42] = 0x40;

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

// this is user shellcode
#include "../build/user.h"

void thread_main() {
	unsigned ret;

	// homebrew enable
	uint32_t *patch;
	DACR_OFF(
		INSTALL_HOOK(hook_sbl_F3411881, (char*)modulemgr_base + 0xb68c); // 3.60
		INSTALL_HOOK(hook_sbl_89CCDA2C, (char*)modulemgr_base + 0xb64c); // 3.60
		INSTALL_HOOK(hook_sbl_BC422443, (char*)modulemgr_base + 0xb67c); // 3.60

		// patch error code 0x80870003 C0-9249-4
		patch = (void*)(scenpdrm_code + 0x8068); // 3.60
		*patch = 0x2500; // mov r5, 0
		patch = (void*)(scenpdrm_code + 0x9994); // 3.60
		*patch = 0x2600; // mov r6, 0

		patch = (void*)(scenpdrm_code + 0x6A38);
		*patch = 0x47702001; // always return 1 in install_allowed
/*
		patch = (void*)(scenpdrm_code + 0x6ADE);
		*patch = 0xFFFFFFFF;
		patch = (void*)(scenpdrm_code + 0x6C94);
		*patch = 0xFFFFFFFF;
		patch = (void*)(scenpdrm_code + 0x6D88);
		*patch = 0xFFFFFFFF;

		patch = (void*)(scesblacmgr_code + 0x1370);
		*patch = 0xFFFFFFFF;
		patch = (void*)(scesblacmgr_code + 0x168C);
		*patch = 0xFFFFFFFF;
*/
		patch = (void*)(scesblacmgr_code + 0x700);
		*patch = 0x47702001; // mov r0, #1 ; bx lr
	);
	SceCpuForDriver_9CB9F0CE_flush_icache((char*)modulemgr_base + 0xb640, 0x80); // should cover all patched exports
	SceCpuForDriver_9CB9F0CE_flush_icache(scenpdrm_code + 0x6000, 0x4000); // and npdrm patches
	SceCpuForDriver_9CB9F0CE_flush_icache(scesblacmgr_code, 0x2000);
	// end homebrew enable

	// takeover the web browser

	unsigned data[0xE8/4];
	data[0] = sizeof(data);
	SceProcessmgrForDriver_0AFF3EAE(pid, data);
	DACR_OFF(ppid = data[5];);
	LOG("WebBrowser PID: 0x%x\n", ppid);

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
		if (strcmp(info.name, "SceWebBrowser") == 0)
			DACR_OFF(SceWebBrowser_base = info.segments[0].vaddr);
		else if (strcmp(info.name, "SceLibKernel") == 0)
			DACR_OFF(SceLibKernel_base = info.segments[0].vaddr);
		else if (strcmp(info.name, "SceDriverUser") == 0)
			DACR_OFF(SceDriverUser_base = info.segments[0].vaddr);
	}
}

void takeover_web_browser() {
	unsigned ret;
	unsigned popt[0x58/4];
	for (int i = 0; i < 0x58/4; ++i)
		popt[i] = 0;
	popt[0] = sizeof(popt);
	popt[2] = 0xA0000000;
	popt[9] = ppid; // allocate in webbrocess space
	ret = sceKernelAllocMemBlockForKernel("", 0xC20D050, 0x4000, popt);
	LOG("alloc memblock ret = 0x%x\n", ret);
	if (ret & 0x80000000)
		return;
	unsigned base = 0;
	ret = sceKernelGetMemBlockBaseForKernel(ret, &base);
	LOG("getbase ret = 0x%x base = 0x%x\n", ret, base);

	// inject the code
	unrestricted_memcpy_for_pid(ppid, base, build_user_bin, (build_user_bin_len + 0x10) & ~0xF);
	LOG("code injected\n");

	int thread = sceKernelCreateThreadForPid(ppid, "", base|1, 64, 0x4000, 0x800000, 0, 0);
	LOG("create thread 0x%x\n", thread);

	unsigned args[] = { SceWebBrowser_base, SceLibKernel_base, SceDriverUser_base };
	ret = sceKernelStartThread_089(thread, sizeof(args), args);
	LOG("sceKernelStartThread_089 ret 0x%x\n", ret);
}

void resolve_imports(unsigned sysmem_base) {
	unsigned ret;

	module_info_t *sysmem_info = find_modinfo(sysmem_base, "SceSysmem");

	// 3.60-specific offsets here, used to find Modulemgr from just sysmem base
	LOG("sysmem base: 0x%08x\n", sysmem_base);
	void *sysmem_data = (void*)(*(u32_t*)((u32_t)(sysmem_base) + 0x26a68) - 0xA0);
	LOG("sysmem data base: 0x%08x\n", sysmem_data);
	DACR_OFF(modulemgr_base = (void*)(*(u32_t*)((u32_t)(sysmem_data) + 0x438c) - 0x40););
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
	module_info_t *threadmgr_info = 0, *sblauthmgr_info = 0, *processmgr_info;
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
		if (strcmp(info.name, "SceSblACMgr") == 0) {
			DACR_OFF(scesblacmgr_code = (u32_t)info.segments[0].vaddr;);
		}
		if (strcmp(info.name, "SceNetPs") == 0) {
			scenet_code = (u32_t)info.segments[0].vaddr;
			scenet_data = (u32_t)info.segments[1].vaddr;
		}
		if (strcmp(info.name, "SceProcessmgr") == 0)
			processmgr_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceProcessmgr");
	}

	LOG("threadmgr_info: 0x%08x | sblauthmgr_info: 0x%08x | scenet_code: 0x%08x | scenet_data: 0x%08x\n", threadmgr_info, sblauthmgr_info, scenet_code, scenet_data);
	LOG("scenpdrm_code: 0x%08x\n", scenpdrm_code);

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

	resolve_imports(sysmem_base);
	thread_main();
	takeover_web_browser();

	LOG("Kill current thread =>");
	LOG("sceKernelExitDeleteThread at 0x%08x\n", sceKernelExitDeleteThread);
	ret = sceKernelExitDeleteThread();
	LOG("??? ret = 0x%08x\n", ret);
	LOG("something broke...");

	while(1) {}
}
