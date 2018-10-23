# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project (CoordinateTransformer)

# 设置编译选项 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(GDAL REQUIRED)


message(STATUS "GDAL library status:")
message(STATUS "    version: ${GDAL_VERSION}")
message(STATUS "    libraries: ${GDAL_LIBRARY}")
message(STATUS "    include path: ${GDAL_INCLUDE_DIRS}")

include_directories(${GDAL_INCLUDE_DIRS})

# 指定生成目标
aux_source_directory(. DIR_SRCS)
add_executable(CoordinateTransformer ${DIR_SRCS} )

target_link_libraries(CoordinateTransformer ${GDAL_LIBRARY})