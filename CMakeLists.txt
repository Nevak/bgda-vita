cmake_minimum_required(VERSION 3.14)

if (NOT DEFINED ENV{VITASDK} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "Please define VITASDK env var to point to your SDK path "
		  "or set CMAKE_TOOLCHAIN_FILE.")
endif ()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
	CACHE PATH "Toolchain file")

project(bgda C CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

# Vita application configuration
set(VITA_APP_NAME "Balrdur's Gate: Dark Alliance")
set(VITA_TITLEID "BGDA00001")
set(VITA_VPKNAME "bgda")
set(VITA_VERSION "01.00")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

# Optional: used for "send" and "dump" targets
set(PSVITAIP "192.168.1.57" CACHE STRING "PSVita IP (for FTP access)") #China
#set(PSVITAIP "192.168.110.38" CACHE STRING "PSVita IP (for FTP access)") #China devkit
set(PARSECOREPATH "/vita-dev/vita-parse-core/main.py" CACHE STRING "vita-parse-core main.py")

# Game-specific definitions
set(DATA_PATH "ux0:data/bgda/" CACHE STRING "Path to data (with trailing /)")
set(SO_PATH "${DATA_PATH}lib/armeabi-v7a/libdarkalliance.so" CACHE STRING "Path to .so")
set(SO_PATH_LIBCPP "${DATA_PATH}lib/armeabi-v7a/libc++_shared.so" CACHE STRING "Path to .so")
set(SO_PATH_LIBXMV "${DATA_PATH}lib/armeabi-v7a/libxmv.so" CACHE STRING "Path to .so")
set(SO_PATH_JBEJNI "${DATA_PATH}lib/armeabi-v7a/libjbe_jni.so" CACHE STRING "Path to .so")
set(SO_PATH_LIBCPUFEATURES "${DATA_PATH}lib/armeabi-v7a/libcpufeatures.so" CACHE STRING "Path to .so")

option(USE_SCELIBC_IO "Use SceLibcBridge for IO and some other std functions" ON)
if (USE_SCELIBC_IO)
  add_definitions(-DUSE_SCELIBC_IO)
endif()

add_definitions(-DDATA_PATH="${DATA_PATH}" 
				-DSO_PATH="${SO_PATH}"
				-DSO_PATH_LIBCPP="${SO_PATH_LIBCPP}"
				-DSO_PATH_LIBXMV="${SO_PATH_LIBXMV}"
				-DSO_PATH_JBEJNI="${SO_PATH_JBEJNI}"
				-DSO_PATH_LIBCPUFEATURES="${SO_PATH_LIBCPUFEATURES}"
				)

# makes sincos, sincosf, etc. visible
add_definitions(-D_GNU_SOURCE -D__POSIX_VISIBLE=999999)

# Unique debug definition for loader's logging to not interfere with anything
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG_SOLOADER)
endif()

# Option to enable profiler instrumentation
option(RAZOR_PROFILER_ENABLED "Enable function instrumentation for profiling" OFF)

# Base compiler flags without instrumentation
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wl,-q,--wrap,malloc,--wrap,memalign,--wrap,free,--wrap,calloc,--wrap,realloc -Wl,--allow-multiple-definition -O3 -ffast-math -mfloat-abi=softfp -Wno-deprecated -mfpu=neon -march=armv7-a -mtune=cortex-a9 -ftree-vectorize -fsingle-precision-constant -fno-omit-frame-pointer -save-temps")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++20 -Wno-write-strings -Wno-psabi -Wl,-q,--wrap,memcpy,--wrap,memset,--wrap,malloc,--wrap,memalign,--wrap,free,--wrap,calloc,--wrap,realloc -O3 -fno-omit-frame-pointer")

# Add instrumentation flags only if profiler is enabled
if(RAZOR_PROFILER_ENABLED)
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finstrument-functions -finstrument-functions-exclude-function-list=__cyg_profile_func_enter,__cyg_profile_func_exit")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finstrument-functions -finstrument-functions-exclude-file-list=third_party/**,allocators/** -finstrument-functions-exclude-function-list=__cyg_profile_func_enter,__cyg_profile_func_exit")
  add_definitions(-DPROFILER_ENABLED)
endif()

add_executable(${CMAKE_PROJECT_NAME}
			   source/main.c
			   source/dynlib.c
			   #source/dll_psp2.c
			   source/falso_jni_impl.c
			   source/patch.c
			   source/reimpl/ctype_patch.c
			   source/reimpl/egl.c
			   source/reimpl/env.c
			   source/reimpl/errno.c
			   source/reimpl/io.c
			   source/reimpl/ioctl.c
			   source/reimpl/log.c
			   source/reimpl/mem.c
			   source/reimpl/pthr.c
			   source/reimpl/sys.c
			   #source/utils/ogg_patch.c
			   source/utils/vorbis_patch.c
			   #source/utils/ffmpeg_patch.c
			   source/utils/prof.c
			   source/utils/dialog.c
			   source/utils/glutil.c
			   source/utils/init.c
			   source/utils/logger.c
			   source/utils/settings.c
			   source/utils/utils.c
			   #source/remap_section.c
			   #source/utils/trophies.c
			   lib/falso_jni/FalsoJNI.c
			   lib/falso_jni/FalsoJNI_ImplBridge.c
			   lib/falso_jni/FalsoJNI_Logger.c
			   lib/falso_jni/converter.c
			   lib/AFakeNative/polling/pseudo_eventfd.cpp
			   lib/AFakeNative/polling/pseudo_pipe.cpp
			   lib/AFakeNative/PseudoEpoll.cpp
			   lib/AFakeNative/AAssetManager.cpp
			   lib/AFakeNative/AStorageManager.cpp
			   lib/AFakeNative/AFakeNative_Utils.cpp
			   lib/AFakeNative/ANativeWindow.cpp
			   lib/AFakeNative/ASensor.cpp
			   lib/AFakeNative/AConfiguration.cpp
			   lib/AFakeNative/AInput.cpp
			   lib/AFakeNative/ALooper.cpp
			   lib/AFakeNative/utils/controls.cpp
			   lib/AFakeNative/utils/sensors.cpp
			   lib/AFakeNative/ANativeActivity.cpp
			   lib/sha1/sha1.c
			   lib/fios/fios.c
			   lib/so_util/so_util.c)

add_subdirectory(lib/libc_bridge)
add_dependencies(${CMAKE_PROJECT_NAME} SceLibcBridge)

target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
						   PUBLIC ${CMAKE_SOURCE_DIR}/lib
						   PUBLIC ${CMAKE_SOURCE_DIR}/lib/fmod/include
						   )

target_include_directories(${CMAKE_PROJECT_NAME}
						   PUBLIC ${CMAKE_SOURCE_DIR}/source
						   )

target_link_libraries(${CMAKE_PROJECT_NAME}
					  -Wl,--whole-archive pthread -Wl,--no-whole-archive
					  stdc++
					  SceLibKernel_stub
					  #sndfile
                      #vorbis
                      #vorbisfile
					  #ogg
					  #FLAC
					  #vorbisidec
					  openal
					  vitaGL
					  vitashark
					  SceShaccCgExt
					  taihen_stub
					  SceShaccCg_stub
					  z
					  m
					  ScePgf_stub
					  mathneon
					  kubridge_stub
					  SceHid_stub
					  SceTouch_stub
					  SceAppMgr_stub
					  SceAppUtil_stub
					  SceAudio_stub
					  SceAudioIn_stub
					  SceCommonDialog_stub
					  SceSysmodule_stub
					  ${CMAKE_BINARY_DIR}/lib/libc_bridge/stubs/libSceLibcBridge_stub.a
					  SceDisplay_stub
					  SceKernelDmacMgr_stub
					  SceFios2_stub
					  SceCtrl_stub
					  SceGxm_stub
					  ScePower_stub
					  SceTouch_stub
					  SceVshBridge_stub
					  )

if(RAZOR_PROFILER_ENABLED)
target_link_libraries(${CMAKE_PROJECT_NAME}
	profilerino
	ScePerf_stub
	SceRazorHud_stub
	SceRazorCapture_stub
)
endif()

vita_create_self(eboot.bin ${CMAKE_PROJECT_NAME} UNSAFE NOASLR UNCOMPRESSED GEN_EXPORTS exports.txt)
vita_create_vpk(${VITA_VPKNAME}.vpk ${VITA_TITLEID} eboot.bin
				VERSION ${VITA_VERSION}
				NAME ${VITA_APP_NAME}
				FILE ${CMAKE_SOURCE_DIR}/extras/cpuinfo cpuinfo
                FILE ${CMAKE_SOURCE_DIR}/extras/meminfo meminfo
				FILE ${CMAKE_SOURCE_DIR}/extras/cpuinfo_max_freq cpuinfo_max_freq
				FILE ${CMAKE_SOURCE_DIR}/extras/possible possible
				FILE ${CMAKE_SOURCE_DIR}/extras/present present
				${CMAKE_SOURCE_DIR}/extras/livearea/icon0.png sce_sys/icon0.png
				${CMAKE_SOURCE_DIR}/extras/livearea/pic0.png sce_sys/pic0.png
				${CMAKE_SOURCE_DIR}/extras/livearea/startup.png sce_sys/livearea/contents/startup.png
				${CMAKE_SOURCE_DIR}/extras/livearea/bg0.png sce_sys/livearea/contents/bg0.png
				${CMAKE_SOURCE_DIR}/extras/livearea/template.xml sce_sys/livearea/contents/template.xml
				${CMAKE_SOURCE_DIR}/extras/livearea/TROPHY.TRP sce_sys/trophy/BGDA00001_00/TROPHY.TRP
				)

add_custom_target(send
				  COMMAND echo destroy | nc ${PSVITAIP} 1338
				  COMMAND echo destroy | nc ${PSVITAIP} 1338
				  COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
				  COMMAND nm --without-symbol-versions --demangle bgda > sym.txt
				  COMMAND echo "nm output saved to sym.txt"
				  COMMAND curl -T sym.txt ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
				  COMMAND echo launch ${VITA_TITLEID} | nc ${PSVITAIP} 1338
				  DEPENDS ${VITA_VPKNAME}.vpk-vpk
				  )

add_custom_target(send_kvdb
				  COMMAND echo destroy | nc ${PSVITAIP} 1338
				  COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
				  COMMAND echo launch ux0:/app/${VITA_TITLEID}/eboot.bin | nc ${PSVITAIP} 1338
				  DEPENDS ${VITA_VPKNAME}.vpk-vpk
				  )

add_custom_target(dump
				  COMMAND ${CMAKE_SOURCE_DIR}/extras/scripts/get_dump.sh ${PSVITAIP}
				  COMMAND python ${PARSECOREPATH} --sym ${CMAKE_BINARY_DIR}/sym.txt /vita-dev/soulcalibur_vita/build/coredump /vita-dev/soulcalibur_vita/build/bgda -s 128
				  )

add_custom_target(dump_local
				  COMMAND bash ${CMAKE_SOURCE_DIR}/extras/scripts/get_dump_local.sh
				  COMMAND python ${PARSECOREPATH} --sym ${CMAKE_BINARY_DIR}/sym.txt /vita-dev/soulcalibur_vita/build/coredump /vita-dev/soulcalibur_vita/build/bgda -s 128
				  )

add_custom_target(reboot
				  COMMAND echo reboot | nc ${PSVITAIP} 1338
				  )
