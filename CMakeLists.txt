cmake_minimum_required(VERSION 3.0)

project(Suduku
	LANGUAGES C)
	
set(CMAKE_C_STANDARD 89)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)
include_directories(${OpenMP_INCLUDE_DIR})

#find_package(OpenMPI REQUIRED)
#include_directories(${OpenMPI_INCLUDE_DIR})

add_executable(generate generate.c)

#add_executable(suduku suduku.c)
#target_link_libraries(suduku ${OpenMP_LIBRARY})

add_executable(suduku2 suduku2.c)
target_link_libraries(suduku2 ${OpenMP_LIBRARY})