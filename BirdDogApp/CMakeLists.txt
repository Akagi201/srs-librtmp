CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(
    BirdDog_Ndi_Over_Rtmp_Url_PublishApp
    VERSION 1.0.0
    DESCRIPTION "BirdDog Ndi over RTMP url publishing application (NDI over RTMP)."
    LANGUAGES C CXX
)

SET(LIBS_3RD_PARTY ${CMAKE_SOURCE_DIR}/3rdParty)

SET(LIBS_3RD_PARTY_NDI_PATH         ${LIBS_3RD_PARTY}/ndi/libs)
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

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/rtmplibrary/src/libs
	${INCLUDE_3RD_PARTY_NDI})

SET(SRC_FILE_LISTS  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )
SET(CMAKE_C_COMPILER gcc  )
SET(CMAKE_CXX_COMPILER g++ )
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(BDRtmpPublishApp ${SRC_FILE_LISTS} )

#supplied libraries
add_library(libndi STATIC IMPORTED)
set_property(TARGET libndi PROPERTY IMPORTED_LOCATION ${LIB_NDI_SHARED})

target_link_libraries(BDRtmpPublishApp  libndi srs_librtmp_static)

# cmake -DCMAKE_BUILD_TYPE=debug ..
# cmake --build .
