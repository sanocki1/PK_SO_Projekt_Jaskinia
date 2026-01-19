cmake_minimum_required(VERSION 4.2)
project(projektJaskiniaSO C)

set(CMAKE_C_STANDARD 23)

include_directories(include)

add_executable(projektJaskiniaSO src/main.c)
target_compile_definitions(projektJaskiniaSO PRIVATE PROCESS_NAME="[Main]")

add_executable(guard src/guard.c)
target_compile_definitions(guard PRIVATE PROCESS_NAME="[Guard]")

add_executable(cashier src/cashier.c)
target_compile_definitions(cashier PRIVATE PROCESS_NAME="[Cashier]")

add_executable(guide src/guide.c)
target_compile_definitions(guide PRIVATE PROCESS_NAME="[Guide]")

add_executable(visitor src/visitor.c)
target_compile_definitions(visitor PRIVATE PROCESS_NAME="[Visitor]")

add_dependencies(projektJaskiniaSO guard cashier guide visitor)