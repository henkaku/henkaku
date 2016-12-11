cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(HENkaku)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

if (NOT ${RELEASE})
  add_definitions(-DENABLE_LOGGING)
endif()

add_executable(user
  user.c
)

add_executable(kernel
  kernel.c
)

target_link_libraries(user
  taihen_stub
  SceLibKernel_stub
  SceIofilemgr_stub
)

target_link_libraries(kernel
  gcc
  taihenForKernel_stub
  SceSysmemForDriver_stub
  SceSysclibForDriver_stub
  SceIofilemgrForDriver_stub
  SceDebugForDriver_stub
  SceModulemgrForKernel_stub
  SceThreadmgrForDriver_stub
)

set_target_properties(kernel
  PROPERTIES LINK_FLAGS "-nostdlib"
  COMPILE_FLAGS "-D__VITA_KERNEL__"
)

set_target_properties(user
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(henkaku.skprx kernel
  CONFIG ${CMAKE_SOURCE_DIR}/kernel.yml
)
vita_create_stubs(henkaku-stubs kernel ${CMAKE_SOURCE_DIR}/kernel.yml
  KERNEL
)
vita_create_self(henkaku.suprx user
  CONFIG ${CMAKE_SOURCE_DIR}/user.yml
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/henkaku-stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)
