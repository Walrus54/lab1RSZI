cmake_minimum_required(VERSION 3.12)

project(crypto_manager LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
            src/encoder.cpp)

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
                      PUBLIC logger
                             Qt5::Core
                             OpenSSL::Crypto)

if(COMMAND set_default_warnings)
  set_default_warnings(${PROJECT_NAME})
endif()
