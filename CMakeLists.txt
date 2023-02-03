cmake_minimum_required(VERSION 3.1...3.24)
project(DMA_Bench VERSION 1.0)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3 -Wall -fcommon -march=native -fPIC")

# set(ENV{PKG_CONFIG_PATH} "/opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig/")
find_package(PkgConfig REQUIRED)
message("${PKG_CONFIG_PATH}")
pkg_search_module(DOCA_ARGP REQUIRED doca-argp)




add_library(params STATIC src/params.c)
target_link_libraries(params ${DOCA_LIBRARIES})
target_link_directories(params PUBLIC ${DOCA_LIBRARY_DIRS})
target_include_directories(params PUBLIC ${DOCA_INCLUDE_DIRS})


add_executable(main src/main.c)
target_link_libraries(main params ${DOCA_LIBRARIES})
target_link_directories(main PUBLIC ${DOCA_LIBRARY_DIRS})
target_include_directories(main PUBLIC ${DOCA_INCLUDE_DIRS})
target_compile_options(main PUBLIC "-D DOCA_ALLOW_EXPERIMENTAL_API")