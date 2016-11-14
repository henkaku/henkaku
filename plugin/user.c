#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <taihen.h>
#include "henkaku.h"
#include "../build/version.c"

#define DISPLAY_VERSION (0x3600000)

static henkaku_config_t config;

static SceUID g_hooks[4];

static tai_hook_ref_t g_sceKernelGetSystemSwVersion_SceSettings_hook;
static int sceKernelGetSystemSwVersion_SceSettings_patched(SceKernelFwInfo *info) {
  int ret;
  int ver_major;
  int ver_minor;
  ret = TAI_CONTINUE(int, g_sceKernelGetSystemSwVersion_SceSettings_hook, info);
  ver_major = ((DISPLAY_VERSION >> 24) & 0xF) + 10 * (DISPLAY_VERSION >> 28);
  ver_minor = ((DISPLAY_VERSION >> 16) & 0xF) + 10 * ((DISPLAY_VERSION >> 20) & 0xF);
  if (BETA_RELEASE) {
    sceClibSnprintf(info->versionString, 16, "%d.%02d \xE5\xA4\x89\xE9\x9D\xA9-%d\xCE\xB2%d", ver_major, ver_minor, HENKAKU_RELEASE, BETA_RELEASE);
  } else {
    sceClibSnprintf(info->versionString, 16, "%d.%02d \xE5\xA4\x89\xE9\x9D\xA9-%d", ver_major, ver_minor, HENKAKU_RELEASE);
  }
  return ret;
}

static tai_hook_ref_t g_update_check_hook;
static int update_check_patched(int a1, int a2, int *a3, int a4, int a5) {
  TAI_CONTINUE(int, g_update_check_hook, a1, a2, a3, a4, a5);
  *a3 = 0;
  return 0;
}

static tai_hook_ref_t g_game_update_check_hook;
static int game_update_check_patched(int newver, int *needsupdate) {
  TAI_CONTINUE(int, g_game_update_check_hook, newver, needsupdate);
  *needsupdate = 0;
  return 0;
}

static tai_hook_ref_t g_app_start_hook;
static int app_start(SceSize argc, const void *args) {
  int ret;
  LOG("before app start");
  //LOG("data: %p", g_app_start_hook);
  //LOG("%x %x %x", *(int *)g_app_start_hook, *(int *)(g_app_start_hook+4), *(int *)(g_app_start_hook+8));
  ret = TAI_CONTINUE(int, g_app_start_hook, argc, args);
  LOG("app_start: %x", ret);
  /*
  g_hooks[1] = taiHookFunctionImport(&g_sceKernelGetSystemSwVersion_SceSettings_hook, 
                                      "SceSettings", 
                                      0xEAED1616, // SceModulemgr
                                      0x5182E212, 
                                      sceKernelGetSystemSwVersion_SceSettings_patched);
  LOG("sceKernelGetSystemSwVersion hook: %x", g_hooks[1]);
  */
  return ret;
}

static int load_config_user(void) {
  SceUID fd;
  int rd;
  fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0);
  if (fd >= 0) {
    rd = sceIoRead(fd, &config, sizeof(config));
    sceIoClose(fd);
    if (rd == sizeof(config)) {
      if (config.magic == HENKAKU_CONFIG_MAGIC) {
        if (config.version >= 8) {
          return 0;
        } else {
          LOG("config version too old");
        }
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
  config.allow_unsafe_hb = 0;
  config.use_spoofed_version = 1;
  config.spoofed_version = SPOOF_VERSION;
  return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
  tai_module_info_t info;
  LOG("loading HENkaku config for user");
  load_config_user();
  /*
  g_hooks[0] = taiHookFunctionExport(&g_app_start_hook, 
                                      "SceSettings", 
                                      0x00000000, // default exports
                                      0x935CD196, // module_start
                                      app_start);
  LOG("app_start hook: %x", g_hooks[0]);
  */
  g_hooks[1] = taiHookFunctionImport(&g_sceKernelGetSystemSwVersion_SceSettings_hook, 
                                      "SceSettings", 
                                      0xEAED1616, // SceModulemgr
                                      0x5182E212, 
                                      sceKernelGetSystemSwVersion_SceSettings_patched);
  LOG("sceKernelGetSystemSwVersion hook: %x", g_hooks[1]);
  g_hooks[2] = g_hooks[3] = -1;
  if (config.use_psn_spoofing) {
    info.size = sizeof(info);
    if (taiGetModuleInfo("SceShell", &info) >= 0) {
      // we don't have a nice clean way of doing PSN spoofing (update prompt disable) so 
      // we are stuck with hard coding offsets. Since module NID is different for each 
      // version and retail/dex/test unit, this should allow us to specify different 
      // offsets.
      switch (info.module_nid) {
        case 0x0552F692: { // retail 3.60 SceShell
          g_hooks[2] = taiHookFunctionOffset(&g_update_check_hook, 
                                             info.modid, 
                                             0,         // segidx
                                             0x363de8,  // offset
                                             1,         // thumb
                                             update_check_patched);
          g_hooks[3] = taiHookFunctionOffset(&g_game_update_check_hook, 
                                             info.modid, 
                                             0,         // segidx
                                             0x37beda,  // offset
                                             1,         // thumb
                                             game_update_check_patched);
          break;
        }
        case 0x6CB01295: { // PDEL 3.60 SceShell thanks to anonymous for offsets
          g_hooks[2] = taiHookFunctionOffset(&g_update_check_hook, 
                                             info.modid, 
                                             0,         // segidx
                                             0x12c882,  // offset
                                             1,         // thumb
                                             update_check_patched);
          g_hooks[3] = taiHookFunctionOffset(&g_game_update_check_hook, 
                                             info.modid, 
                                             0,         // segidx
                                             0x36df3e,  // offset
                                             1,         // thumb
                                             game_update_check_patched);
          break;
        }
        default: {
          LOG("SceShell NID %X not recognized, skipping PSN spoofing patches", info.module_nid);
        }
      }
    }
  } else {
    LOG("skipping psn spoofing patches");
  }
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  LOG("stopping module");
  // free hooks that didn't fail
  //if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], g_app_start_hook);
  if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], g_sceKernelGetSystemSwVersion_SceSettings_hook);
  if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_update_check_hook);
  if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_game_update_check_hook);
  return SCE_KERNEL_STOP_SUCCESS;
}
