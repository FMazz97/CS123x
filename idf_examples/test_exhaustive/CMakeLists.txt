cmake_minimum_required(VERSION 3.16)

#useful for local development prior to publication
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components/cs123x")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_exhaustive)