cmake_minimum_required(VERSION 4.2)
project(cave_sim C)

set(CMAKE_C_STANDARD 23)

include_directories(include)

add_executable(cave_sim src/main.c src/utils.c)
target_compile_definitions(cave_sim PRIVATE
        PROCESS_NAME="[Main]")

add_executable(guard src/guard.c src/utils.c)
target_compile_definitions(guard PRIVATE
        PROCESS_NAME="[Guard]"
        PROCESS_COLOR=COLOR_YELLOW)

add_executable(cashier src/cashier.c src/utils.c)
target_compile_definitions(cashier PRIVATE
        PROCESS_NAME="[Cashier]"
        PROCESS_COLOR=COLOR_BLUE)

add_executable(guide src/guide.c src/utils.c)
target_compile_definitions(guide PRIVATE
        PROCESS_NAME="[Guide]"
        PROCESS_COLOR=COLOR_MAGENTA)

add_executable(visitor src/visitor.c src/utils.c)
target_compile_definitions(visitor PRIVATE
        PROCESS_NAME="[Visitor]"
        PROCESS_COLOR=COLOR_CYAN)

add_dependencies(cave_sim guard cashier guide visitor)