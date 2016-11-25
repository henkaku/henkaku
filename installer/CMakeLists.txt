cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(installer)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(installer.elf
	compress.c
	crc32.c
	installer.c
)

target_link_libraries(installer.elf
  gcc
  SceLibKernel_stub
  SceIoFilemgr_stub
  SceKernelThreadmgr_stub
  SceSysmem_stub
  SceProcessmgr_stub
  SceDisplay_stub
  SceNet_stub
  SceNetCtl_stub
  SceHttp_stub
  SceShellSvc_stub
  SceCtrl_stub
  SceSysmodule_stub
  SceAppMgr_stub
  ScePromoterUtil_stub
)

set_target_properties(installer.elf
  PROPERTIES LINK_FLAGS "-nostdlib"
)

add_custom_target(installer.self ALL
	COMMAND vita-elf-create installer.elf installer.velf
	COMMAND vita-make-fself installer.velf installer.self
)
add_dependencies(installer.self installer.elf)
