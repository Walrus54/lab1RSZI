cmake_minimum_required(VERSION 3.12)

project(RecursiveFileFolderEncryptor
        VERSION 1.0.0
        DESCRIPTION "Recursive file/folder encryptor and decryptor"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RECURSIVE_ENCRYPTOR_BUILD_TESTS "Build unit tests" OFF)

set(OPENSSL_REQUIRED_VERSION
    "3.3.6"
    CACHE STRING "Required OpenSSL version (default from assignment)")

find_package(Qt5 5.12 REQUIRED COMPONENTS Core)
find_package(OpenSSL ${OPENSSL_REQUIRED_VERSION} REQUIRED COMPONENTS Crypto)

if(RECURSIVE_ENCRYPTOR_BUILD_TESTS)
  include(CTest)
endif()

# Общая функция предупреждений компилятора для всех целей.
function(set_default_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

add_subdirectory(logger)
add_subdirectory(crypto_manager)
add_subdirectory(recursive_stepper)

add_executable(recursive_encryptor
               main.cpp)

target_link_libraries(recursive_encryptor
                      PRIVATE logger
                              crypto_manager
                              recursive_stepper
                              Qt5::Core)

set_default_warnings(recursive_encryptor)
