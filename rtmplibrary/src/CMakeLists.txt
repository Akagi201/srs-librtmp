cmake_minimum_required(VERSION 2.8.4)

project(srs_librtmp)

AUX_SOURCE_DIRECTORY(core SOURCE_FILES)
AUX_SOURCE_DIRECTORY(kernel SOURCE_FILES)
AUX_SOURCE_DIRECTORY(libs SOURCE_FILES)
AUX_SOURCE_DIRECTORY(protocol SOURCE_FILES)

INCLUDE_DIRECTORIES(core kernel libs protocol)

add_library(srs_librtmp_static STATIC ${SOURCE_FILES})

set_target_properties(srs_librtmp_static PROPERTIES OUTPUT_NAME "bdsrs_librtmp")

#ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/research/librtmp)