cmake_minimum_required(VERSION 3.21)

project(
	VanillaDirectionalDodge
	VERSION 1.0.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# triplet x64-windows-static-md: статические библиотеки vcpkg, динамический CRT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE-NG/CMakeLists.txt")
	message(FATAL_ERROR
		"extern/CommonLibSSE-NG is empty. Run:\n"
		"  git submodule add -b ng https://github.com/alandtse/CommonLibSSE-NG.git extern/CommonLibSSE-NG\n"
		"  git submodule update --init --recursive")
endif()

# тесты CommonLib нам не нужны (иначе потребуется Catch2)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/CommonLibSSE-NG)

# хелпер add_commonlibsse_plugin() (генерирует SKSEPlugin_Version / SKSEPlugin_Query)
include(extern/CommonLibSSE-NG/cmake/CommonLibSSE.cmake)

add_commonlibsse_plugin(
	${PROJECT_NAME}
	NAME "VanillaDirectionalDodge"
	AUTHOR "Contrus1408"
	VERSION ${PROJECT_VERSION}
	USE_ADDRESS_LIBRARY
	SOURCES src/main.cpp
)
