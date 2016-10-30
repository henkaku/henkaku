#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/io/fcntl.h>
#include <stdio.h>
#include <taihen.h>

#define OFFSET_PATCH_ARG 168

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

//static int g_enable_unsafe_homebrew;

/*
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
        if (g_enable_unsafe_homebrew) {
          *(uint32_t *)(args + OFFSET_PATCH_ARG) = 0x40;
        } else {
          ret = 0x80020001;
        }
      }
    }
  }
  return ret;
}
*/

int module_start(SceSize argc, const void *args) {
  /*
  g_hooks[0] = taiHookFunctionImportForKernel(KERNEL_PID, 
                                              &g_parse_headers_hook, 
                                              "SceKernelModulemgr", 
                                              0x7ABF5135, // SceSblAuthMgrForKernel
                                              0xF3411881, 
                                              parse_headers_patched);
  */
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  return SCE_KERNEL_STOP_SUCCESS;
}

int _start(void) {
  sceIoOpenForDriver("ux0:test", 0, 0);
  return 0;
}
