# Yes the CMAKE file produces warnings, but it is just cmake being mad that not every standard was followed
# So for now it does not matter

cmake_minimum_required (VERSION 3.5.1)

add_definitions("-Wall" "-Wextra" "-Wconversion" "-Wpedantic" "-O3" "-march=native" "-mtune=native" "-std=c++20")

# Main lib

project (bmpxx
  LANGUAGES CXX
)

include_directories(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_library(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} m)

# Test exec

project(
  bmpxx_test
  LANGUAGES CXX
)

include_directories(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)

file(GLOB TEST_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)

add_executable(${PROJECT_NAME} ${TEST_FILES})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/test/images $<TARGET_FILE_DIR:${PROJECT_NAME}>/images)

target_link_libraries(${PROJECT_NAME} bmpxx m)





