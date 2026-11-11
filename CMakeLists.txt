cmake_minimum_required(VERSION 3.10)
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "VITASDK not set")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "")
project(bb3_vita C)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
# softfp matters: guest code passes floats in integer registers (see NOTES.md)
if(NOT DEFINED BQ_OPT)
  set(BQ_OPT "-O2")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${BQ_OPT} -mfpu=neon -Wl,-q -include ${CMAKE_SOURCE_DIR}/src/bqlog.h")
add_executable(bb3 src/vita_main.c src/extra_shims.c src/macho.c src/shims.c src/libc_shims.c src/sys_shims.c src/objc_rt.c src/gl_shims.c src/pvrtc.c src/sjis.c src/audio.c)
# vitaGL (+ vitaShaRK runtime shader compiler) and the Vita stubs it needs; only stubs that exist in this SDK are linked
set(VGL_STUB_CANDIDATES SceGxm SceDisplay SceCommonDialog SceAppMgr SceSysmodule SceKernelDmacMgr SceDmac SceShaccCg SceTouch SceThreadmgr SceModulemgr SceProcessmgr taihen)
set(VGL_STUBS "")
foreach(l ${VGL_STUB_CANDIDATES})
  if(EXISTS "$ENV{VITASDK}/arm-vita-eabi/lib/lib${l}_stub.a")
    list(APPEND VGL_STUBS ${l}_stub)
  else()
    message(STATUS "stub not found (skipped): ${l}_stub")
  endif()
endforeach()
target_link_libraries(bb3 -Wl,--whole-archive pthread -Wl,--no-whole-archive
  -Wl,--whole-archive vitaGL -Wl,--no-whole-archive
  -Wl,--start-group vitashark SceShaccCgExt mathneon ${VGL_STUBS} -Wl,--end-group stdc++
  m kubridge_stub ScePower_stub SceTouch_stub SceCtrl_stub SceAudio_stub SceIofilemgr_stub SceSysmem_stub SceLibKernel_stub)
vita_create_self(bb3.self bb3 UNSAFE)
# App icon: made from your own IPA (tools/make_icon.py) - Gameloft's art is not part of the repository
if(EXISTS ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png AND EXISTS ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png)
  vita_create_vpk(bb3.vpk BBRK00003 bb3.self VERSION 01.00 NAME "Block Breaker 3"
    FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
    FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml)
elseif(EXISTS ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png)
  vita_create_vpk(bb3.vpk BBRK00003 bb3.self VERSION 01.00 NAME "Block Breaker 3"
    FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png)
else()
  vita_create_vpk(bb3.vpk BBRK00003 bb3.self VERSION 01.00 NAME "Block Breaker 3")
endif()
