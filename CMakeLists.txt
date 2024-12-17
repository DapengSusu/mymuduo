cmake_minimum_required(VERSION 3.5)

project(mymuduo VERSION 0.0.1)

# 设置so库生成在项目目录的lib文件夹下
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# 配置编译，使用c++11标准
# -fPIC：编译器编译时启用位置无关代码的特性，建议在编译动态链接库时始终使用-fPIC编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")

# Debug or Release
set(CMAKE_BUILD_TYPE Debug)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g")
endif()

# 指定参与编译的源文件
aux_source_directory(. SRC_LIST)

# mymuduo最终编译成so动态库
add_library(mymuduo SHARED ${SRC_LIST})
