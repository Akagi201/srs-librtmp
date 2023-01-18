CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(
    BIRDDOG_RTMP_PUB_APP
    VERSION 1.0.0
    DESCRIPTION "BirdDog RTMP publishing application (NDI over RTMP)."
    LANGUAGES C CXX
)

SET(SRS_LIBRTMP_L ../../winbuild/Debug/srs_librtmp.lib )


SET(LIBS_3RD_PARTY ${CMAKE_SOURCE_DIR}/3rdParty)

SET(LIBS_3RD_PARTY_NDI_PATH         ${LIBS_3RD_PARTY}/ndi)
SET(LIBS_3RD_PARTY_NDI_WIN_LIB      ${LIBS_3RD_PARTY_NDI_PATH}/Processing.NDI.Lib.Advanced.x64.lib)
SET(LIBS_3RD_PARTY_NDI_LINUX        ${LIBS_3RD_PARTY_NDI_PATH}/libndi-x86_64.so.5)
SET(LIBS_3RD_PARTY_NDI_ARM32        ${LIBS_3RD_PARTY_NDI_PATH}/libndi-armv7l.so.5)
SET(LIBS_3RD_PARTY_NDI_ARM64        ${LIBS_3RD_PARTY_NDI_PATH}/libndi-aarch64.so.5)
SET(INCLUDE_3RD_PARTY_NDI ${LIBS_3RD_PARTY}/ndi/include)

if(MSVC)
    SET(LIB_NDI_SHARED  ${LIBS_3RD_PARTY_NDI_WIN_LIB})
else()
    if(_cmparm32lo)
        SET(LIB_NDI_SHARED   ${LIBS_3RD_PARTY_NDI_ARM32})
    elseif(_cmparm64lo)
        SET(LIB_NDI_SHARED   ${LIBS_3RD_PARTY_NDI_ARM64})
    else()
        SET(LIB_NDI_SHARED   ${LIBS_3RD_PARTY_NDI_LINUX})
    endif()
endif()

INCLUDE_DIRECTORIES( ../../src/libs/srs_librtmp.hpp ${CMAKE_SOURCE_DIR}/ndi)
INCLUDE_DIRECTORIES(${INCLUDE_3RD_PARTY_NDI})

SET(SRC_FILE_LISTS  ${CMAKE_SOURCE_DIR}/research/librtmp/bd_srs_h264_publish.cpp )
SET(CMAKE_C_COMPILER gcc  )
SET(CMAKE_CXX_COMPILER g++ )
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(BDRtmpPublishApp ${SRC_FILE_LISTS} )
if(NOT MSVC)
    target_link_libraries(BDRtmpPublishApp dl)
endif()

#supplied libraries
add_library(libndi STATIC IMPORTED)
set_property(TARGET libndi PROPERTY IMPORTED_LOCATION ${LIB_NDI_SHARED})

add_library(librtmpstatic STATIC IMPORTED)
set_property(TARGET librtmpstatic PROPERTY IMPORTED_LOCATION ${SRS_LIBRTMP_L})

target_link_libraries(BDRtmpPublishApp  librtmpstatic)

# cmake -DCMAKE_BUILD_TYPE=debug ..
# cmake --build .
