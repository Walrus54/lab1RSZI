cmake_minimum_required(VERSION 3.12)

project(RecursiveFileFolderEncryptor
        VERSION 0.1.0
        DESCRIPTION "Recursive file/folder encryptor prototype"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core)

function(set_default_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

add_subdirectory(recursive_stepper)

add_executable(recursive_encryptor
               main.cpp)

target_link_libraries(recursive_encryptor
                      PRIVATE recursive_stepper
                              Qt5::Core)

set_default_warnings(recursive_encryptor)
