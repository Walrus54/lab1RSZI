cmake_minimum_required(VERSION 3.12)

project(RecursiveFileFolderEncryptor
        VERSION 0.1.0
        DESCRIPTION "Recursive file/folder encryptor prototype"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core)

set(OPENSSL_REQUIRED_VERSION
    "3.3.6"
    CACHE STRING "Required OpenSSL version (default from assignment)")

find_package(OpenSSL  REQUIRED COMPONENTS Crypto)

function(set_default_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

add_subdirectory(recursive_stepper)
add_subdirectory(logger)
add_subdirectory(crypto_manager)

add_executable(recursive_encryptor
               main.cpp)

target_link_libraries(recursive_encryptor
                      PRIVATE recursive_stepper
                              logger
                              crypto_manager
                              Qt5::Core)

set_default_warnings(recursive_encryptor)
