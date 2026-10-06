cmake_minimum_required(VERSION 3.20)
project(Engine2D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # needed to link static libs into the Python module

include(FetchContent)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)   # old third-party CMakeLists still configure under CMake 4
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SFML    GIT_REPOSITORY https://github.com/SFML/SFML.git          GIT_TAG 2.6.1)
FetchContent_Declare(box2d   GIT_REPOSITORY https://github.com/erincatto/box2d.git    GIT_TAG v2.4.1)
FetchContent_Declare(EnTT    GIT_REPOSITORY https://github.com/skypjack/entt.git      GIT_TAG v3.13.2)
FetchContent_Declare(pybind11 GIT_REPOSITORY https://github.com/pybind/pybind11.git   GIT_TAG v2.13.6)
FetchContent_MakeAvailable(SFML box2d EnTT pybind11)

add_library(engine_core STATIC engine/src/Engine.cpp)
target_include_directories(engine_core PUBLIC engine/include)
target_link_libraries(engine_core PUBLIC box2d EnTT::EnTT sfml-graphics sfml-window sfml-system)

pybind11_add_module(pyengine bindings/bindings.cpp)
target_include_directories(pyengine PRIVATE bindings)
target_link_libraries(pyengine PRIVATE engine_core)

# Host exe: C++ owns the engine, Python is embedded
add_executable(engine_host app/main.cpp)
target_include_directories(engine_host PRIVATE bindings)
target_link_libraries(engine_host PRIVATE engine_core pybind11::embed)
set_target_properties(pyengine PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python)

enable_testing()
add_executable(test_physics tests/test_physics.cpp)
target_link_libraries(test_physics PRIVATE engine_core)
add_test(NAME physics COMMAND test_physics)
