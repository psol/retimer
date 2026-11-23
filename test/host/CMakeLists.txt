# Tests sur l'hôte (docs/PLAN.md, section 4.2).
#
# Projet CMake indépendant d'ESP-IDF : il ne compile que les composants sans
# dépendance au matériel, avec le compilateur de la machine (clang natif sur
# le Mac). C'est aussi la preuve qu'ils restent portables.
#
#   cmake -S test/host -B test/host/build
#   cmake --build test/host/build
#   ctest --test-dir test/host/build --output-on-failure
cmake_minimum_required(VERSION 3.16)
project(retimer_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(RETIMER_SANITIZERS "AddressSanitizer et UndefinedBehaviorSanitizer" ON)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(COMPONENTS ${ROOT}/components)

set(WARNINGS -Wall -Wextra -Werror -Wshadow -Wconversion
    -Wno-sign-conversion)
if(RETIMER_SANITIZERS)
  set(SANITIZERS -fsanitize=address,undefined -fno-omit-frame-pointer
      -fno-sanitize-recover=all)
endif()

# Unity, copié tel quel (test/host/unity, licence MIT) : pas d'avertissements
# stricts sur son code.
add_library(unity STATIC unity/unity.c)
target_include_directories(unity PUBLIC unity)
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
target_compile_options(unity PRIVATE ${SANITIZERS})
target_link_options(unity PUBLIC ${SANITIZERS})

# Composants sans dépendance au matériel.
set(LOGIC_SOURCES "")
set(LOGIC_INCLUDES "")
foreach(component constants adjust kitchen_timer input render)
  file(GLOB _sources ${COMPONENTS}/${component}/*.c)
  list(APPEND LOGIC_SOURCES ${_sources})
  list(APPEND LOGIC_INCLUDES ${COMPONENTS}/${component}/include)
endforeach()

add_library(logic STATIC ${LOGIC_SOURCES})
target_include_directories(logic PUBLIC ${LOGIC_INCLUDES})
target_compile_options(logic PRIVATE ${WARNINGS} ${SANITIZERS})

enable_testing()

# Un exécutable par fichier test_*.c.
file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.c)
foreach(test_file ${TEST_FILES})
  get_filename_component(name ${test_file} NAME_WE)
  add_executable(${name} ${test_file} ${CMAKE_CURRENT_SOURCE_DIR}/helpers/helpers.c)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/helpers)
  target_link_libraries(${name} PRIVATE logic unity)
  target_compile_options(${name} PRIVATE ${WARNINGS} ${SANITIZERS})
  target_compile_definitions(${name} PRIVATE
    RETIMER_REFERENCES="${CMAKE_CURRENT_SOURCE_DIR}/references"
    RETIMER_OUTPUTS="${CMAKE_CURRENT_BINARY_DIR}/screens")
  add_test(NAME ${name} COMMAND ${name})
endforeach()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/screens)
