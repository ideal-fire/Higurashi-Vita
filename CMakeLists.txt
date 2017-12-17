cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME HIGURASHI)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Higurahsi When They Cry Hou")
set(VITA_TITLEID  "HIGURASHI")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
  ./Include
)

link_directories(
  ./lib
)

add_executable(${SHORT_NAME}
  stolenCode/soloud_c.cpp
  src/main.c
)

target_link_libraries(${SHORT_NAME}
  -lGeneralGoodVita
  -lsoloud
  -lpthread
  -lScePower_stub
  -lvita2d
  -lSceDisplay_stub
  -lSceGxm_stub
  -lSceSysmodule_stub
  -lSceCtrl_stub
  -lScePgf_stub
  -lSceCommonDialog_stub
  -lSceAudio_stub
  -lSceTouch_stub
  -lfreetype
  -lpng
  -ljpeg
  -lz
  -lm
  -lc
  -lluaVita
  -lm
)

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME})
vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
