cmake_minimum_required(VERSION 3.20)
project(aerionuaserver CXX)
set(CMAKE_CXX_STANDARD 17)
include(FetchContent)

find_package(open62541)
find_package(re2)
find_package(fmt)
find_package(nlohmann_json)
find_package(Threads)
find_package(reproc++ CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(efsw CONFIG REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/external/tray")

add_executable(aerionuaserver main.cpp external/loguru/loguru.cpp)

if (WIN32)
	target_compile_options(aerionuaserver PRIVATE /utf-8 /w14165 /w44242 /w44254 /w44263 /w34265 /w34287 /w44296 /w44365 /w44388 /w44464 /w14545 /w14546 /w14547 /w14549 /w14555 /w34619 /w34640 /w24826 /w14905 /w14906 /w14928 /w45038 /W4 /permissive- /volatile:iso /Zc:preprocessor /Zc:__cplusplus /Zc:externConstexpr /Zc:throwingNew /EHsc /wd4100)
else()
	target_compile_options(aerionuaserver PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wcast-qual -Wformat=2 -Wundef -Werror=float-equal -Wshadow -Wcast-align -Wunused -Wno-unused-parameter -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough -Wextra-semi -Woverloaded-virtual -Wnon-virtual-dtor -Wold-style-cast)
endif()

target_link_libraries(aerionuaserver PRIVATE re2::re2 open62541::open62541 fmt::fmt nlohmann_json::nlohmann_json tray::tray reproc++ cppzmq efsw::efsw)
target_include_directories(aerionuaserver PRIVATE include external)