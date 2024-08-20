cmake_minumum_required(VERSION 3.0.0)
project(MyCMakeProject VERSION 0.1.0 LANGUAGE  CXX)

include(CTest)
enable_testing()

add_executable(MyCMakeProject server.cpp)

set_property(TARGET MyCMakeProject PROPERTY CXX_STANDARD 17)