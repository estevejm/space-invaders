cmake_minimum_required(VERSION 3.30)
project(spaceinvaders C)

set(CMAKE_C_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH "bin")

add_executable(spaceinvaders src/main.c src/space_invaders.c src/i8080.c src/memory.c)
