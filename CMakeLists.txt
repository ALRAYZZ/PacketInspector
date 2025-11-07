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
  "core/CaptureEngine.h" 
  "core/CaptureEngine.cpp"
  "core/PacketParser.h"
 "gui/panels/CaptureControlPanel.h" "gui/panels/CaptureControlPanel.cpp" "core/PacketInfo.h" "core/PacketInfo.cpp" "gui/panels/PacketListPanel.h" "gui/panels/PacketListPanel.cpp" "core/PacketParser.cpp" "gui/panels/PacketDetailPanel.h" "gui/panels/PacketDetailPanel.cpp" "core/PingEngine.h" "core/PingEngine.cpp" "gui/panels/PingToolPanel.h" "gui/panels/PingToolPanel.cpp" "core/PacketCrafterEngine.h" "core/PacketCrafterEngine.cpp" "gui/panels/PacketCrafterPanel.h")

# PCAP / NPCAP integration
set(NPCAP_ROOT "D:/.CODING/C++/Libraries/Npcap" CACHE PATH "Path to Npcap SDK")

if (WIN32)
find_path(PCAP_INCLUDE_DIR pcap.h HINTS "${NPCAP_ROOT}/Include" ENV NPCAP_ROOT PATH_SUFFIXES Include)
target_link_libraries(PacketInspector PRIVATE iphlpapi ws2_32)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
set(_npcap_lib_subdir "Lib/x64")
else()
set(_npcap_lib_subdir "Lib")
endif()
find_library(WPCAP_LIBRARY NAMES wpcap
HINTS "${NPCAP_ROOT}/${_npcap_lib_subdir}" ENV NPCAP_ROOT
)
find_library(PACKET_LIBRARY NAMES Packet
HINTS "${NPCAP_ROOT}/${_npcap_lib_subdir}" ENV NPCAP_ROOT)

if(NOT PCAP_INCLUDE_DIR OR NOT WPCAP_LIBRARY OR NOT PACKET_LIBRARY)
message(FATAL_ERROR "Could not find Npcap/WinPcap SDK. Please set NPCAP_ROOT to the Npcap SDK path.")
endif()

target_include_directories(PacketInspector PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}
	${PCAP_INCLUDE_DIR}
)
target_link_libraries(PacketInspector PRIVATE "${WPCAP_LIBRARY}" "${PACKET_LIBRARY}"
Ws2_32 Iphlpapi
)
else()
find_package(Pcap REQUIRED)
if(PCAP_FOUND)
target_include_directories(PacketInspector PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(PacketInspector PRIVATE ${PCAP_LIBRARIES})
endif()
endif()

# Link libraries
target_link_libraries(PacketInspector PRIVATE
	SDL2::SDL2
	SDL2::SDL2main
	imgui::imgui
	OpenGL::GL
)
