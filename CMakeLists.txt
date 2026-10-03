cmake_minimum_required(VERSION 3.20)
project(poll_sim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(poll_sim_core
  src/population.cpp
  src/sampling.cpp
  src/poll.cpp
  src/experiment.cpp
  src/estimate.cpp
  src/stats.cpp
)
target_include_directories(poll_sim_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_executable(poll_sim src/main.cpp)
target_link_libraries(poll_sim PRIVATE poll_sim_core)

foreach(target poll_sim_core poll_sim)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 $<$<CONFIG:Release>:/O2>)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic $<$<CONFIG:Release>:-O3>)
  endif()
endforeach()
