cmake_minimum_required(VERSION 4.3)
project(CinamaEditor C)

include_directories("../Spark/libs/include")
include_directories("src")
link_directories("../Spark/libs/lib")
set(CMAKE_C_STANDARD 99)

add_executable(CinamaEditor src/main.c
        src/core/theme.c
        src/fx/audio.c
        src/fx/fx.c
        src/ui/ui.c
        src/buffer/line.c
        src/buffer/syntax.c
        src/core/event.c
        src/render/renderer.c
        src/modules/input/input.c
        src/modules/view/camera.c
        src/modules/view/cursor.c
        src/modules/buffer/document.c
        src/modules/io/disk.c
        src/render/canvas.c
        src/ui/menu_bar.c
        src/buffer/grammar.c
        src/modules/project/workspace.c
        src/ui/sidebar.c
        src/ui/palette.c
        src/ui/completion.c)
target_link_libraries(CinamaEditor raylib m winmm)
