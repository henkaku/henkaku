#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <taihen.h>
#include "henkaku.h"

int module_start(SceSize argc, const void *args) {
  tai_start_t *caller;

  sceIoOpen("ux0:test", 0, 0);
  if (argc >= sizeof(*caller)) {
    caller = (tai_start_t *)args;
    return SCE_KERNEL_START_SUCCESS;
  } else {
    return SCE_KERNEL_START_FAILED;
  }
}

int module_stop(SceSize argc, const void *args) {
  return SCE_KERNEL_STOP_SUCCESS;
}
