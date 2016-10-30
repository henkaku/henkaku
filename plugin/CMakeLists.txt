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

add_executable(user.elf
	user.c
)

add_executable(kernel.elf
	kernel.c
)

target_link_libraries(user.elf
	gcc
)

target_link_libraries(kernel.elf
	gcc
)

#set_target_properties(kernel.elf
#	PROPERTIES LINK_FLAGS "-nostdlib"
#)

add_custom_target(henkaku.skprx ALL
	COMMAND vita-elf-create kernel.elf kernel.velf
	COMMAND vita-make-fself kernel.velf henkaku.skprx
)
add_dependencies(henkaku.skprx kernel.elf)

add_custom_target(henkaku.suprx ALL
	COMMAND vita-elf-create user.elf user.velf
	COMMAND vita-make-fself user.velf henkaku.suprx
)
add_dependencies(henkaku.suprx user.elf)
