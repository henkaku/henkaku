cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(HENkaku)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

if (NOT ${RELEASE})
  add_definitions(-DENABLE_LOGGING)
endif()

add_executable(user.elf
	user.c
)

add_executable(kernel.elf
	kernel.c
)

target_link_libraries(user.elf
  taihen_stub
  SceLibKernel_stub
  SceIofilemgr_stub
)

target_link_libraries(kernel.elf
	gcc
  taihenForKernel_stub
  SceSysmemForDriver_stub
  SceSysclibForDriver_stub
  SceIofilemgrForDriver_stub
  SceDebugForDriver_stub
  SceModulemgrForKernel_stub
)

set_target_properties(kernel.elf
  PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

set_target_properties(user.elf
  PROPERTIES LINK_FLAGS "-nostdlib"
)

add_custom_target(henkaku.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/kernel.yml kernel.elf kernel.velf ${CMAKE_SOURCE_DIR}/taihen.json
	COMMAND vita-make-fself kernel.velf henkaku.skprx
)
add_dependencies(henkaku.skprx kernel.elf)

add_custom_target(henkaku.suprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/user.yml user.elf user.velf ${CMAKE_SOURCE_DIR}/taihen.json
	COMMAND vita-make-fself user.velf henkaku.suprx
)
add_dependencies(henkaku.suprx user.elf)

add_custom_target(henkaku-libs ALL
  COMMAND vita-elf-export kernel kernel.elf ${CMAKE_SOURCE_DIR}/kernel.yml kernel.json
  COMMAND vita-libs-gen kernel.json henkaku-libs
  COMMAND make -C henkaku-libs
)

add_dependencies(henkaku-libs henkaku.elf)
