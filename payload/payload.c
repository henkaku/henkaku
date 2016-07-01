#include <inttypes.h>

#define DEBUG 0

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

void payload(uint32_t sysmem_addr) {
	// find sysmem base, etc
	uint32_t sysmem_base = sysmem_addr;
	uint32_t ret;
	void (*debug_print_local)(char *s, ...) = (void*)(sysmem_base + 0x1A155); // 3.60

	module_info_t *sysmem_info = find_modinfo(sysmem_addr, "SceSysmem");
	void (*SceCpuForDriver_9CB9F0CE_flush_icache)(void *addr, uint32_t size) = find_export(sysmem_info, 0x9CB9F0CE);

	DACR_OFF(
		debug_print = debug_print_local;
	);

	LOG("+++ Entered kernel payload +++\n");
	
	// 3.60-specific offsets here, used to find Modulemgr from just sysmem base
	LOG("sysmem base: 0x%08x\n", sysmem_base);
	void *sysmem_data = (void*)(*(u32_t*)((u32_t)(sysmem_base) + 0x26a68) - 0xA0);
	LOG("sysmem data base: 0x%08x\n", sysmem_data);
	void *modulemgr_base = (void*)(*(u32_t*)((u32_t)(sysmem_data) + 0x438c) - 0x40);
	LOG("modulemgr base: 0x%08x\n", modulemgr_base);
	// end of 3.60-specific offsets

	module_info_t *modulemgr_info = find_modinfo((u32_t)modulemgr_base, "SceKernelModulemgr");
	LOG("modulemgr modinfo: 0x%08x\n", modulemgr_info);

	int (*sceKernelGetModuleListForKernel)() = find_export(modulemgr_info, 0x97CF7B4E);
	int (*sceKernelGetModuleInfoForKernel)() = find_export(modulemgr_info, 0xD269F915);
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
	module_info_t *threadmgr_info = 0, *sblauthmgr_info = 0;
	u32_t scenet_code = 0, scenet_data = 0;
	for (int i = 0; i < modlist_records; ++i) {
		info.size = sizeof(info);
		ret = sceKernelGetModuleInfoForKernel(0x10005, modlist[i], &info);
		if (strcmp(info.name, "SceKernelThreadMgr") == 0)
			threadmgr_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceKernelThreadMgr");
		if (strcmp(info.name, "SceSblAuthMgr") == 0)
			sblauthmgr_info = find_modinfo((u32_t)info.segments[0].vaddr, "SceSblAuthMgr");
		if (strcmp(info.name, "SceNetPs") == 0) {
			scenet_code = (u32_t)info.segments[0].vaddr;
			scenet_data = (u32_t)info.segments[1].vaddr;
		}
	}

	LOG("threadmgr_info: 0x%08x | sblauthmgr_info: 0x%08x | scenet_code: 0x%08x | scenet_data: 0x%08x\n", threadmgr_info, sblauthmgr_info, scenet_code, scenet_data);

	LOG("Fixup: unlock SceNetPs global mutex ");
	// 3.60
	int (*sce_psnet_bnet_mutex_unlock)() = (void*)(scenet_code + 0x2a098|1);
	void *mutex = (void*)((u32_t)scenet_data + 0x850);
	// end
	ret = sce_psnet_bnet_mutex_unlock(mutex);
	LOG("=> ret = 0x%08x\n", ret);

	// homebrew enable
	DACR_OFF(
		hook_resume_sbl_F3411881 = find_export(sblauthmgr_info, 0xF3411881);
		hook_resume_sbl_89CCDA2C = find_export(sblauthmgr_info, 0x89CCDA2C);
		hook_resume_sbl_BC422443 = find_export(sblauthmgr_info, 0xBC422443);
		INSTALL_HOOK(hook_sbl_F3411881, (char*)modulemgr_base + 0xb68c); // 3.60
		INSTALL_HOOK(hook_sbl_89CCDA2C, (char*)modulemgr_base + 0xb64c); // 3.60
		INSTALL_HOOK(hook_sbl_BC422443, (char*)modulemgr_base + 0xb67c); // 3.60
	);
	SceCpuForDriver_9CB9F0CE_flush_icache((char*)modulemgr_base + 0xb640, 0x80); // should cover all patched exports
	// end homebrew enable

	LOG("Kill current thread =>");
	int (*sceKernelExitDeleteThread)() = find_export(threadmgr_info, 0x1D17DECF);
	LOG("sceKernelExitDeleteThread at 0x%08x\n", sceKernelExitDeleteThread);
	ret = sceKernelExitDeleteThread();
	LOG("??? ret = 0x%08x\n", ret);
	LOG("something broke...");

	while(1) {}
}

void __attribute__ ((section (".text.start"), naked)) start(void) {
	__asm__ (
		"add sp, #0x1000 \n"
		"mov r0, r4      \n"
		"b payload       \n"
	);
}
