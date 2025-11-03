# CMakeList.txt : CMake project for PacketInspector, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Auto-use vcpkg if available
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("PacketInspector")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Add source to this project's executable
add_executable (PacketInspector
  main.cpp
  "gui/GuiManager.cpp" 
  "gui/GuiManager.h"
  "gui/backends/imgui_impl_sdl2.cpp"
  "gui/backends/imgui_impl_opengl3.cpp"
)

# Link libraries
target_link_libraries(PacketInspector PRIVATE
	SDL2::SDL2
	SDL2::SDL2main
	imgui::imgui
	OpenGL::GL
)
