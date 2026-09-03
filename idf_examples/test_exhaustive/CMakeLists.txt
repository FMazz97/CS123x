cmake_minimum_required(VERSION 3.16)

#utile per lo sviluppo locale prima di un'eventuale pubblicazione
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../..")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_exhaustive)