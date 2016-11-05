#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <taihen.h>
#include "henkaku.h"
#include "../build/version.c"

#define OFFSET_PATCH_ARG (168)

typedef struct {
  uint32_t magic;                 /* 53434500 = SCE\0 */
  uint32_t version;               /* header version 3*/
  uint16_t sdk_type;              /* */
  uint16_t header_type;           /* 1 self, 2 unknown, 3 pkg */
  uint32_t metadata_offset;       /* metadata offset */
  uint64_t header_len;            /* self header length */
  uint64_t elf_filesize;          /* ELF file length */
  uint64_t self_filesize;         /* SELF file length */
  uint64_t unknown;               /* UNKNOWN */
  uint64_t self_offset;           /* SELF offset */
  uint64_t appinfo_offset;        /* app info offset */
  uint64_t elf_offset;            /* ELF #1 offset */
  uint64_t phdr_offset;           /* program header offset */
  uint64_t shdr_offset;           /* section header offset */
  uint64_t section_info_offset;   /* section info offset */
  uint64_t sceversion_offset;     /* version offset */
  uint64_t controlinfo_offset;    /* control info offset */
  uint64_t controlinfo_size;      /* control info size */
  uint64_t padding;               
} __attribute__((packed)) self_header_t;

typedef struct {
  uint64_t authid;                /* auth id */
  uint32_t vendor_id;             /* vendor id */
  uint32_t self_type;             /* app type */
  uint64_t padding;               /* UNKNOWN */
} __attribute__((packed)) app_info_t;

static henkaku_config_t config;

static SceUID g_hooks[7];

static tai_hook_ref_t g_parse_headers_hook;
static int parse_headers_patched(int ctx, const void *headers, size_t len, void *args) {
  self_header_t *self;
  app_info_t *info;
  int ret;

  ret = TAI_CONTINUE(int, g_parse_headers_hook, ctx, headers, len, args);
  if (len >= sizeof(self_header_t) && len >= sizeof(app_info_t)) {
    self = (self_header_t *)headers;
    if (self->appinfo_offset <= len - sizeof(app_info_t)) {
      info = (app_info_t *)(headers + self->appinfo_offset);
      LOG("authid: 0x%llx\n", info->authid);
      if ((info->authid & 0xFFFFFFFFFFFFFFFDLL) == 0x2F00000000000001LL) {
        if (config.allow_unsafe_hb) {
          *(uint32_t *)(args + OFFSET_PATCH_ARG) = 0x40;
        }
      }
    }
  }
  return ret;
}

static tai_hook_ref_t g_some_sysroot_check_hook;
static int some_sysroot_check_patched(void) {
  TAI_CONTINUE(int, g_some_sysroot_check_hook);
  return 1;
}

static tai_hook_ref_t g_some_process_check_patched_hook;
static int some_process_check_patched(void) {
  TAI_CONTINUE(int, g_some_process_check_patched_hook);
  return 0;
}

static tai_hook_ref_t g_some_power_auth_check_hook;
static int some_power_auth_check_patched(void) {
  TAI_CONTINUE(int, g_some_power_auth_check_hook);
  return 1;
}

static tai_hook_ref_t g_load_user_libs_hook;
static int load_user_libs_patched(SceUID pid, void *args, int flags) {
  int ret;
  SceUID mod;
  char titleid[32];

  ret = TAI_CONTINUE(int, g_load_user_libs_hook, pid, args, flags);

  sceKernelGetProcessTitleIdForKernel(pid, titleid, 32);
  LOG("title started: %s", titleid);

  // TODO: replace with configuration system
  if (strncmp(titleid, "NPXS10015", 32) == 0) {
    mod = sceKernelLoadModuleForPid(pid, "ux0:app/MLCL00001/henkaku.suprx", 0x8000, NULL);
    LOG("load henkaku user: %x", mod);
  }

  return ret;
}

static tai_hook_ref_t g_sceKernelGetSystemSwVersion_hook;
static int sceKernelGetSystemSwVersion_patched(SceKernelFwInfo *info) {
  int ret;
  int ver_major;
  int ver_minor;
  ret = TAI_CONTINUE(int, g_sceKernelGetSystemSwVersion_hook, info);
  info->version = config.spoofed_version;
  ver_major = ((config.spoofed_version >> 24) & 0xF) + 10 * (config.spoofed_version >> 28);
  ver_minor = ((config.spoofed_version >> 16) & 0xF) + 10 * ((config.spoofed_version >> 20) & 0xF);
  snprintf(info->versionString, 16, "%d.%02d", ver_major, ver_minor);
  return ret;
}

static tai_hook_ref_t g_SceSblSsUpdateMgr_8E3EC2E1_hook;
static int SceSblSsUpdateMgr_8E3EC2E1_patched(int r0, uintptr_t out) {
  int ver;
  int ret;
  ret = TAI_CONTINUE(int, g_SceSblSsUpdateMgr_8E3EC2E1_hook, r0, out);
  ver = SPOOF_VERSION;
  sceKernelMemcpyKernelToUser(out+4, &ver, 4);
  return ret;
}

static int load_config_kernel(void) {
  SceUID fd;
  int rd;
  fd = sceIoOpenForDriver(CONFIG_PATH, SCE_O_RDONLY, 0);
  if (fd >= 0) {
    rd = sceIoReadForDriver(fd, &config, sizeof(config));
    sceIoCloseForDriver(fd);
    if (rd == sizeof(config)) {
      if (config.magic == HENKAKU_CONFIG_MAGIC) {
        return 0;
      } else {
        LOG("config incorrect magic: %x", config.magic);
      }
    } else {
      LOG("config not right size: %d", rd);
    }
  } else {
    LOG("config file not found");
  }
  // default config
  config.magic = HENKAKU_CONFIG_MAGIC;
  config.version = HENKAKU_RELEASE;
  config.use_psn_spoofing = 1;
  config.allow_unsafe_hb = 1; // TODO: default 0
  config.use_spoofed_version = 1;
  config.spoofed_version = SPOOF_VERSION;
  return 0;
}

int module_start(SceSize argc, const void *args) {
  LOG("loading HENkaku config for kernel");
  load_config_kernel();
  // this hook enables extended permissions for unsafe homebrew + blocking unsafe homebrew if requested
  g_hooks[0] = taiHookFunctionImportForKernel(KERNEL_PID, 
                                              &g_parse_headers_hook, 
                                              "SceKernelModulemgr", 
                                              0x7ABF5135, // SceSblAuthMgrForKernel
                                              0xF3411881, 
                                              parse_headers_patched);
  LOG("parse_headers_hook: %x", g_hooks[0]);
  // the next two hooks enable dynarec by patching a system check and a process check
  g_hooks[1] = taiHookFunctionExportForKernel(KERNEL_PID, 
                                              &g_some_sysroot_check_hook, 
                                              "SceSysmem", 
                                              0x3691DA45, // SceSysrootForKernel
                                              0xF8769E86, 
                                              some_sysroot_check_patched);
  LOG("some_sysroot_check_hook: %x", g_hooks[1]);
  /*
  // we remove this because it might not be needed. the MRC at the beginning makes libsubstitute complain. Should file ticket.
  g_hooks[2] = taiHookFunctionExportForKernel(KERNEL_PID, 
                                              &g_some_process_check_patched_hook, 
                                              "SceSysmem", 
                                              0x63A519E5, // SceSysmemForKernel
                                              0xD514BB56, 
                                              some_process_check_patched);
  LOG("some_process_check_patched_hook: %x", g_hooks[2]);
  */
  // this hook patches an auth check in ScePower for enabling overclocking in safe homebrew
  g_hooks[3] = taiHookFunctionImportForKernel(KERNEL_PID, 
                                              &g_some_power_auth_check_hook, 
                                              "ScePower", 
                                              0x9AD8E213, // SceSblACMgrForDriver
                                              0x8612B243, 
                                              some_power_auth_check_patched);
  LOG("some_power_auth_check_hook: %x", g_hooks[3]);
  // this allows us to load henkaku.suprx with SceSettings regardless of ux0:tai/config.txt settings
  // this is because ux0:tai/config.txt is not used if non-safe homebrew is not enabled
  g_hooks[4] = taiHookFunctionExportForKernel(KERNEL_PID, 
                                              &g_load_user_libs_hook, 
                                              "SceKernelModulemgr", 
                                              0xC445FA63, // SceModulemgrForKernel
                                              0x3AD26B43,
                                              load_user_libs_patched);
  LOG("load_user_libs added");
  if (config.use_spoofed_version) {
    // this hook spoofs the system version for the API access
    g_hooks[5] = taiHookFunctionExportForKernel(KERNEL_PID, 
                                                &g_sceKernelGetSystemSwVersion_hook, 
                                                "SceKernelModulemgr", 
                                                0xD4A60A52, // SceModulemgrForDriver
                                                0x5182E212, 
                                                sceKernelGetSystemSwVersion_patched);
    LOG("sceKernelGetSystemSwVersion_hook: %x", g_hooks[4]);
    // this hook spoofs the system version for internal access
    g_hooks[6] = taiHookFunctionExportForKernel(KERNEL_PID, 
                                                &g_SceSblSsUpdateMgr_8E3EC2E1_hook, 
                                                "SceSblUpdateMgr", 
                                                0x31406C49, // SceSblSsUpdateMgr
                                                0x8E3EC2E1, 
                                                SceSblSsUpdateMgr_8E3EC2E1_patched);
    LOG("SceSblSsUpdateMgr_8E3EC2E1_hook: %x", g_hooks[5]);
  } else {
    LOG("skipping version spoofing");
  }
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  taiHookReleaseForKernel(g_hooks[0], g_parse_headers_hook);
  taiHookReleaseForKernel(g_hooks[1], g_some_sysroot_check_hook);
  //taiHookReleaseForKernel(g_hooks[2], g_some_process_check_patched_hook);
  taiHookReleaseForKernel(g_hooks[3], g_some_power_auth_check_hook);
  taiHookReleaseForKernel(g_hooks[4], g_load_user_libs_hook);
  if (config.use_spoofed_version) {
    taiHookReleaseForKernel(g_hooks[5], g_sceKernelGetSystemSwVersion_hook);
    taiHookReleaseForKernel(g_hooks[6], g_SceSblSsUpdateMgr_8E3EC2E1_hook);
  }
  return SCE_KERNEL_STOP_SUCCESS;
}
