cmake_minimum_required(VERSION 3.12)

project(recursive_stepper LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
            src/recursive_processor.cpp)

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
                      PUBLIC Qt5::Core)

if(COMMAND set_default_warnings)
  set_default_warnings(${PROJECT_NAME})
endif()
