cmake_minimum_required(VERSION 4.2)
project(projektJaskiniaSO C)

set(CMAKE_C_STANDARD 23)

add_executable(projektJaskiniaSO src/main.c
        src/cashier.c
        src/guard.c
        src/guide.c
        src/visitor.c)
