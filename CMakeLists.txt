cmake_minimum_required(VERSION 3.10)

project(adrill)

set(ADRILL_VERSION_MAJOR 1)
set(ADRILL_VERSION_MINOR 1)
set(ADRILL_VERSION_PATCH 0)

configure_file (
	source/config.h.in
	"${PROJECT_BINARY_DIR}/config.h"
	@ONLY
)

add_definitions(-DMEM_USE_DLFCN)

include_directories(
    "${PROJECT_BINARY_DIR}"
    source
)

add_subdirectory(mem mem/build)

if (CMAKE_ANDROID_ARCH_ABI MATCHES "^armeabi*")
    set(ADRILL_ARCH arm)
elseif (CMAKE_ANDROID_ARCH_ABI MATCHES "^arm64*")
    set(ADRILL_ARCH arm64)
elseif (CMAKE_ANDROID_ARCH_ABI MATCHES "^x86$")
    set(ADRILL_ARCH x86)
elseif (CMAKE_ANDROID_ARCH_ABI MATCHES "^x86_64$")
    set(ADRILL_ARCH x64)
endif ()

add_executable(adrill
    source/main.cc
    source/selinux.cc
    source/ptrace_wrapper.cc
    source/call_procedure.cc
    source/backend/${ADRILL_ARCH}/call_procedure-${ADRILL_ARCH}.cc
)

target_link_libraries(adrill mem)

set_target_properties(adrill PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
set_target_properties(adrill PROPERTIES LINK_FLAGS "-llog -ldl")
set_target_properties(adrill PROPERTIES ANDROID_STL "c++_static")
