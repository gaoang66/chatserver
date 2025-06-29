# cmake_minimum_required(VERSION 3.0)

# project(main)

# #配置编译选项
# set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#设置可知性文件最终存储路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#头文件搜索路径 include_directories()
#库文件搜索路径 link_directories()

#设置编译的源文件列表
set(SRC_LIST muduo_server.cc)

#把.指定路径下的所有源文件名字放入变量名SRC_LIST
#aux_source_directory(. SRC_LIST)

add_executable(server ${SRC_LIST})

#表示链接的库文件
target_link_libraries(server muduo_net muduo_base pthread)