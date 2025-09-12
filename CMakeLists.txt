cmake_minimum_required(VERSION 3.14)
project(ADLXCli)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  ADLX 
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/ADLX.git 
  GIT_TAG V1.4
)

FetchContent_MakeAvailable(ADLX)

add_library(ADLX OBJECT
  ${adlx_SOURCE_DIR}/SDK/ADLXHelper/Windows/Cpp/ADLXHelper.h
  ${adlx_SOURCE_DIR}/SDK/ADLXHelper/Windows/Cpp/ADLXHelper.cpp
  ${adlx_SOURCE_DIR}/SDK/platform/Windows/WinAPIs.cpp
)

target_include_directories(ADLX
	PUBLIC ${adlx_SOURCE_DIR}
)

add_executable(ADLXCli
	"main.cpp"
)

target_link_libraries(ADLXCli
	PUBLIC ADLX
)