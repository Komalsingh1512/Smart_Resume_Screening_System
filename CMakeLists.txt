cmake_minimum_required(VERSION 3.16)
project(SmartResumeScreening LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(resume_screening
    src/main.cpp
    src/screening_system.cpp
)
target_include_directories(resume_screening PRIVATE include)

if(MSVC)
    target_compile_options(resume_screening PRIVATE /W4)
else()
    target_compile_options(resume_screening PRIVATE -Wall -Wextra -Wpedantic)
endif()
