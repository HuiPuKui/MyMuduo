cmake_minimum_required(VERSION 2.5)
project(mymuduo)

# mymuduo 朢�终编译成 so 动��库，设置动态库的路径，放在根目录的 lib 文件夹下靄1�7
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息 以及 启动 C++11 语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

# 定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)
# 编译生成动��库 mymuduo
add_library(mymuduo SHARED ${SRC_LIST})