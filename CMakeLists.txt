cmake_minimum_required(VERSION 3.16)
project(BOMBERMAN)

set(CMAKE_CXX_STANDARD "20")
set(CMAKE_CXX_FLAGS "-g -std=gnu++2a -Wall -Wextra -Wconversion -Wshadow -O2")

message(STATUS "start running cmake...")

find_package(Boost COMPONENTS program_options REQUIRED)

if(Boost_FOUND)

    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(${Boost_INCLUDE_DIRS})

    add_library(utils utils.cpp)
    add_executable(robots-client robots-client.cpp utils)
    add_executable(robots-server robots-server.cpp)
    #add_executable(client client.cpp)
    add_executable(server server.cpp)
    #add_executable(program_options_test program_options_test.cpp)
    #add_executable(program_options_test_bigger program_options_test_bigger.cpp)

    target_link_libraries(robots-client LINK_PUBLIC ${Boost_LIBRARIES} pthread)
    target_link_libraries(robots-server LINK_PUBLIC ${Boost_LIBRARIES} pthread)
    #target_link_libraries(program_options_test LINK_PUBLIC ${Boost_LIBRARIES})
    #target_link_libraries(program_options_test_bigger LINK_PUBLIC ${Boost_LIBRARIES})
    #target_link_libraries(client LINK_PUBLIC ${Boost_LIBRARIES} pthread)
    target_link_libraries(server LINK_PUBLIC ${Boost_LIBRARIES} pthread)

    install(TARGETS DESTINATION .)
endif()
