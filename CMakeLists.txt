cmake_minimum_required(VERSION 4.0)

project(
    CochleaSim
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SFML)


add_executable(
    cochlea-sim
    source/window_event_handler.cxx
    source/main.cxx
)

target_include_directories(cochlea-sim PRIVATE include)
target_link_libraries(cochlea-sim PRIVATE 
    Threads::Threads
    SFML::Graphics
    SFML::Window
    SFML::System

)