cmake_minimum_required(VERSION 2.8.4)

project(srs_librtmp)

AUX_SOURCE_DIRECTORY(src/core SOURCE_FILES)
AUX_SOURCE_DIRECTORY(src/kernel SOURCE_FILES)
AUX_SOURCE_DIRECTORY(src/libs SOURCE_FILES)
AUX_SOURCE_DIRECTORY(src/protocol SOURCE_FILES)

INCLUDE_DIRECTORIES(src/core src/kernel src/libs src/protocol)

add_library(srs_librtmp_static STATIC ${SOURCE_FILES})

set_target_properties(srs_librtmp_static PROPERTIES OUTPUT_NAME "srs_librtmp")

ADD_SUBDIRECTORY(research/librtmp)