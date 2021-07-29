cmake_minimum_required(VERSION 3.12)
project(python_bindings VERSION ${sqsgenerator_VERSION_MAJOR}.${sqsgenerator_VERSION_MINOR})

find_package (Python3 COMPONENTS Interpreter Development)

message([PYTHON]:INCLUDE_DIRS: ${Python3_INCLUDE_DIRS})
message([PYTHON]:LIBRARY_DIRS: ${Python3_LIBRARY_DIRS})
message([PYTHON]:LIBRARIES: ${Python3_LIBRARIES})
message([PYTHON]:EXECUTABLE: ${Python3_EXECUTABLE})

if(USE_MPI)
    if (MPI_FOUND)
        set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
    endif()
endif()

include_directories(${SQSGENERATOR_INCLUDE_DIR} include)
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})


add_library(data SHARED
        module_data.cpp
        ${SQSGENERATOR_SOURCE_DIR}/atomistics.cpp
        ${SQSGENERATOR_SOURCE_DIR}/result.cpp
        ${SQSGENERATOR_SOURCE_DIR}/rank.cpp
        ${SQSGENERATOR_SOURCE_DIR}/utils.cpp
        ${SQSGENERATOR_SOURCE_DIR}/structure_utils.cpp
        data.hpp
        data.cpp
        )
set_target_properties(data PROPERTIES SUFFIX .so)
set_target_properties(data PROPERTIES PREFIX "")
target_link_libraries(data ${Python3_LIBRARIES} ${Boost_LIBRARIES})

add_library(utils SHARED
        module_utils.cpp
        ${SQSGENERATOR_SOURCE_DIR}/atomistics.cpp
        ${SQSGENERATOR_SOURCE_DIR}/result.cpp
        ${SQSGENERATOR_SOURCE_DIR}/rank.cpp
        ${SQSGENERATOR_SOURCE_DIR}/utils.cpp
        ${SQSGENERATOR_SOURCE_DIR}/structure_utils.cpp
        data.hpp
        data.cpp
        )
add_dependencies(utils data)
set_target_properties(utils PROPERTIES SUFFIX .so)
set_target_properties(utils PROPERTIES PREFIX "")
target_link_libraries(utils ${Python3_LIBRARIES} ${Boost_LIBRARIES})


add_library(iteration SHARED
        module_iteration.cpp
        data.cpp
        iteration.cpp
        ${SQSGENERATOR_SOURCE_DIR}/sqs.cpp
        ${SQSGENERATOR_SOURCE_DIR}/settings.cpp
        ${SQSGENERATOR_SOURCE_DIR}/atomistics.cpp
        ${SQSGENERATOR_SOURCE_DIR}/result.cpp
        ${SQSGENERATOR_SOURCE_DIR}/rank.cpp
        ${SQSGENERATOR_SOURCE_DIR}/utils.cpp
        ${SQSGENERATOR_SOURCE_DIR}/structure_utils.cpp
         )

add_dependencies(iteration data)
set_target_properties(iteration PROPERTIES SUFFIX .so)
set_target_properties(iteration PROPERTIES PREFIX "")
target_link_libraries(iteration pthread ${Python3_LIBRARIES} ${Boost_LIBRARIES})
if (USE_MPI)
    target_link_libraries(iteration ${MPI_CXX_LIBRARIES})
endif()
