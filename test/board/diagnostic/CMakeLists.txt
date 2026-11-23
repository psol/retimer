# Application de diagnostic du CoreInk (docs/verifications-carte.md).
# Vérifie les pilotes un par un, sur la console série :
#   cd test/board/diagnostic
#   idf.py set-target esp32
#   idf.py build flash monitor
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../../components)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(retimer_diagnostic)
