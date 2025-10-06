cmake_minimum_required(VERSION 3.14)
project(ADLXCli)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
else()
    add_compile_options(-O2)
endif()

include(FetchContent)

FetchContent_Declare(
  ADLX 
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/ADLX.git 
  GIT_TAG V1.4
)
FetchContent_MakeAvailable(ADLX)

FetchContent_Declare(
	json 
	URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_library(ADLX OBJECT
  ${adlx_SOURCE_DIR}/SDK/ADLXHelper/Windows/Cpp/ADLXHelper.h
  ${adlx_SOURCE_DIR}/SDK/ADLXHelper/Windows/Cpp/ADLXHelper.cpp
  ${adlx_SOURCE_DIR}/SDK/platform/Windows/WinAPIs.cpp
)

target_include_directories(ADLX
	SYSTEM PUBLIC ${adlx_SOURCE_DIR}
	PUBLIC "include"
)

add_executable(ADLXCli
	"src/main.cpp"
	"app.rc"
)

target_link_libraries(ADLXCli
	PUBLIC ADLX
	PUBLIC nlohmann_json::nlohmann_json
PUBLIC WinToast
)
