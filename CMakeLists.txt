cmake_minimum_required(VERSION 3.0)
project(rta01)

find_package(OpenGL REQUIRED)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory(external)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

include_directories(
	external/glfw/include/
	external/glm-0.9.7.1/
	external/glad-opengl-3.3/include/
		external/assimp/include/
	external/
		external/imgui/
	lab2/
)

add_library(imgui STATIC
		external/imgui/imgui.cpp
		external/imgui/imgui_draw.cpp
		external/imgui/imgui_tables.cpp
		external/imgui/imgui_widgets.cpp
		external/imgui/imgui_impl_glfw.cpp
		external/imgui/imgui_impl_opengl3.cpp
)

link_directories(external/assimp/lib/)

add_executable(rta01
	src/main.cpp
)
target_link_libraries(rta01
	${OPENGL_LIBRARY}
	glfw
	glad
	assimp
		imgui
)
