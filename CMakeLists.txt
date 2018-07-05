cmake_minimum_required(VERSION 3.7)
project(sgm C)

file(GLOB SRC
	"src/*.h"
	"src/*.c"
)

add_executable(${PROJECT_NAME} ${SRC})
