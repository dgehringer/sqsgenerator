cmake_minimum_required(VERSION 3.12)
project(sqsgenerator.core.python)
set(CMAKE_CXX_STANDARD 17)
set(PY_VERSION 3.7)

find_package (Python3 COMPONENTS Interpreter Development)

message([PYTHON]:INCLUDE_DIRS: ${Python3_INCLUDE_DIRS})
message([PYTHON]:LIBRARY_DIRS: ${Python3_LIBRARY_DIRS})
message([PYTHON]:LIBRARIES: ${Python3_LIBRARIES})

include_directories(../include include)
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})

add_library(data SHARED
        data.cpp
        ../src/rank.cpp
        ../src/containers.cpp
        )
set_target_properties(data PROPERTIES SUFFIX .so)
set_target_properties(data PROPERTIES PREFIX "")
target_link_libraries(data pthread ${Python3_LIBRARIES} ${Boost_LIBRARIES})
