cmake_minimum_required(VERSION 4.3)
project(CinamaEditor C)

include_directories("../Spark/libs/include")
include_directories("src")
link_directories("../Spark/libs/lib")
set(CMAKE_C_STANDARD 99)

add_executable(CinamaEditor src/main.c
        src/core/theme.c
        src/editor/editor.c
        src/fx/audio.c
        src/fx/fx.c
        src/ui/ui.c
        src/buffer/line.c
        src/buffer/syntax.c
        src/core/event.c)
target_link_libraries(CinamaEditor raylib m winmm)
