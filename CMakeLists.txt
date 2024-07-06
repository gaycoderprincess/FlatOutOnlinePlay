cmake_minimum_required(VERSION 3.27)
project(FlatOutOnlinePlay)

SET(CMAKE_SYSTEM_NAME Windows)

SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
SET(CMAKE_RANLIB i686-w64-mingw32-ranlib)

SET(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -masm=intel -s -static")

link_directories(${CMAKE_SOURCE_DIR}/../nya-common/lib32)
add_library(FlatOutOnlinePlay SHARED
        ../nya-common/nya_commontimer.cpp
        main.cpp)
target_include_directories(FlatOutOnlinePlay PUBLIC ../nya-common)
target_include_directories(FlatOutOnlinePlay PUBLIC ../nya-common/3rdparty)
target_link_options(FlatOutOnlinePlay PRIVATE "-Wl,--exclude-all-symbols")

set_target_properties(FlatOutOnlinePlay PROPERTIES PREFIX "")
set_target_properties(FlatOutOnlinePlay PROPERTIES SUFFIX "_gcp.dll")

target_link_libraries(FlatOutOnlinePlay ws2_32)
target_link_libraries(FlatOutOnlinePlay wininet)
target_link_libraries(FlatOutOnlinePlay curl zlib bcrypt crypt32 ws2_32)