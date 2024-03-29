cmake_minimum_required(VERSION 3.16)
project(BOMBERMAN)

set(CMAKE_CXX_STANDARD "20")
set(CMAKE_CXX_FLAGS " -std=gnu++20 -Wall -Wextra -Werror -Wconversion -O2")

message(STATUS "start running cmake...")

find_package(Boost COMPONENTS program_options REQUIRED)

if(Boost_FOUND)

    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(${Boost_INCLUDE_DIRS})

    add_executable(robots-client robots-client.cpp definitions.hpp buffer.hpp serialization.hpp utils.hpp)
    add_executable(robots-server robots-server.cpp definitions.hpp buffer.hpp serialization.hpp utils.hpp game.hpp)

    target_link_libraries(robots-client LINK_PUBLIC ${Boost_LIBRARIES} pthread)
    target_link_libraries(robots-server LINK_PUBLIC ${Boost_LIBRARIES} pthread)

else()
    message(FATAL_ERROR "Boost not found")
endif()
