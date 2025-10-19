cmake_minimum_required(VERSION 3.22)

project(zx)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
  main.c
  calculator.c
  calculator.h
  mpextras.c
  mpextras.h
  btree.c
  btree.h
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGMP REQUIRED IMPORTED_TARGET gmp)
target_include_directories(${PROJECT_NAME} PUBLIC ${LIBGMP_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${LIBGMP_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBGMP_LIBRARIES})
pkg_check_modules(LIBMPFR REQUIRED IMPORTED_TARGET mpfr)
target_include_directories(${PROJECT_NAME} PUBLIC ${LIBMPFR_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${LIBMPFR_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBMPFR_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

find_library(MATHLIB m)
if (MATHLIB) 
  target_link_libraries(${PROJECT_NAME} PUBLIC ${MATHLIB})
endif()
