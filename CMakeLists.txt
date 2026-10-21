cmake_minimum_required(VERSION 3.16)
project(SCAPMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE SCAPMS_SOURCES CONFIGURE_DEPENDS
    model/*.cpp controller/*.cpp gui/*.cpp utils/*.cpp)

qt_add_executable(SCAPMS WIN32 main.cpp ${SCAPMS_SOURCES})
target_include_directories(SCAPMS PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SCAPMS PRIVATE Qt6::Widgets)

# Put the sample data next to the executable the first time only, so saved data is never overwritten.
add_custom_command(TARGET SCAPMS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/data/clinic_data.csv
            -DDST=$<TARGET_FILE_DIR:SCAPMS>/clinic_data.csv
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopySampleData.cmake)
