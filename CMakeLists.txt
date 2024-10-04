cmake_minimum_required(VERSION 3.10)

include(FetchContent)

set(TARGET pdf_barcode)
project(${TARGET})

set(ZINT_DEBUG     OFF CACHE BOOL "Set debug compile flags"         FORCE)
set(ZINT_NOOPT     OFF CACHE BOOL "Set no optimize compile flags"   FORCE)
set(ZINT_SANITIZE  OFF CACHE BOOL "Set sanitize compile/link flags" FORCE)
set(ZINT_TEST      OFF CACHE BOOL "Set test compile flag"           FORCE)
set(ZINT_COVERAGE  OFF CACHE BOOL "Set code coverage flags"         FORCE)
set(ZINT_SHARED    OFF CACHE BOOL "Build shared library"            FORCE)
set(ZINT_STATIC    ON  CACHE BOOL "Build static library"            FORCE)
set(ZINT_USE_PNG   ON  CACHE BOOL "Build with PNG support"          FORCE)
set(ZINT_USE_QT    OFF CACHE BOOL "Build with Qt support"           FORCE)
set(ZINT_QT6       OFF CACHE BOOL "If ZINT_USE_QT, use Qt6"         FORCE)
set(ZINT_UNINSTALL ON  CACHE BOOL "Add uninstall target"            FORCE)
set(ZINT_FRONTEND  OFF CACHE BOOL "Build frontend"                  FORCE)

FetchContent_Declare(
  PDFHummus
  GIT_REPOSITORY https://github.com/galkahana/PDF-Writer.git
  GIT_TAG        v4.6.6
  FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(PDFHummus)

FetchContent_Declare(
  zint
  GIT_REPOSITORY https://github.com/zint/zint.git
  GIT_TAG        2.13.0
  FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(zint)

include_directories(${CMAKE_SOURCE_DIR})

list(APPEND SRCS "main.cpp")

set(COMPILER_FLAGS "")
set(COMPILER_FLAGS "${COMPILER_FLAGS} -ggdb")
set(COMPILER_FLAGS "${COMPILER_FLAGS} -O2")

set(COMPILER_FLAGS "${COMPILER_FLAGS} -g2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}")

add_executable(${TARGET} ${SRCS})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ARGS --only-keep-debug ${${TARGET}_BINARY_DIR}/${PROJECT_NAME} ${${TARGET}_BINARY_DIR}/${PROJECT_NAME}.debug)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} ${${TARGET}_BINARY_DIR}/${PROJECT_NAME})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ARGS --add-gnu-debuglink=${${TARGET}_BINARY_DIR}/${PROJECT_NAME}.debug ${${TARGET}_BINARY_DIR}/${PROJECT_NAME})

add_dependencies(${TARGET} PDFHummus::PDFWriter)
add_dependencies(${TARGET} zint-static)

target_link_libraries (${TARGET} PDFHummus::PDFWriter)
target_link_libraries (${TARGET} zint-static)
