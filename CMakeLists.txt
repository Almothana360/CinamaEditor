cmake_minimum_required(VERSION 4.3)
project(CinamaEditor C)

include_directories("../Spark/libs/include")
link_directories("../Spark/libs/lib")
set(CMAKE_C_STANDARD 99)

add_executable(CinamaEditor main.c)
target_link_libraries(CinamaEditor raylib m winmm)
