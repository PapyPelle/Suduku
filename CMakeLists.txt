cmake_minimum_required(VERSION 3.0)

project(Suduku
	LANGUAGES C)
	
set(CMAKE_C_STANDARD 90)

find_package(OpenMP REQUIRED)

#find_package(OpenMPI REQUIRED)
#include_directories(${OpenMPI_INCLUDE_DIR})

add_executable(generate generate.c)

add_executable(suduku suduku.c)
target_compile_options(suduku PRIVATE ${OpenMP_C_FLAGS})
#target_link_libraries(suduku ${OpenMP_C_FLAGS})

add_executable(suduku2 suduku2.c)
target_compile_options(suduku2 PRIVATE ${OpenMP_C_FLAGS})
#target_link_libraries(suduku2 ${OpenMP_C_FLAGS})