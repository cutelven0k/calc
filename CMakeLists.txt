cmake_minimum_required(VERSION 3.16)
project(calc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    mathlib
    GIT_REPOSITORY https://github.com/cutelven0k/mathlib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(mathlib)

add_executable(calc
    src/main.cpp
)

target_link_libraries(calc PRIVATE mathlib::mathlib)
target_include_directories(calc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;--warnings-as-errors=*;--format-style=file")

find_program(CLANG_FORMAT NAMES clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE FORMAT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )
    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT} -i ${FORMAT_SOURCES} --style=WebKit
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-format"
    )
endif()

include(GNUInstallDirs)
install(TARGETS calc
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
