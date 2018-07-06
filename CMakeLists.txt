cmake_minimum_required(VERSION 3.7)
project(sgm C)

find_package(SDL2 REQUIRED)

include_directories(
	${SDL2_INCLUDE_DIRS}
)

file(GLOB SRC
	"src/*.h"
	"src/*.c"
)

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME}
	${SDL2_LIBRARIES}
)
