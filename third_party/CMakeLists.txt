include(FetchContent)

option(MPACK_DEBUG "Compile mpack with MPACK_DEBUG=1 and min buffer size" OFF)

if(DD_APPSEC_BUILD_HELPER)
    set(LIBDDWAF_TESTING OFF)
    add_subdirectory(libddwaf EXCLUDE_FROM_ALL)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929)

    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE MSGPACK_C_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/msgpack-c/src/*.c)
    add_library(msgpack_c STATIC ${MSGPACK_C_SOURCES})
    set_target_properties(msgpack_c PROPERTIES POSITION_INDEPENDENT_CODE 1)
    target_include_directories(msgpack_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/msgpack-c/include/)
    target_compile_definitions(msgpack_c INTERFACE MSGPACK_CXX17=ON)

    file(GLOB_RECURSE CPPBASE64_C_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cpp-base64/base64.cpp)
    add_library(cpp-base64 STATIC ${CPPBASE64_C_SOURCES})
    target_include_directories(cpp-base64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cpp-base64)
endif()

if(DD_APPSEC_BUILD_EXTENSION)
    hunter_add_package(gflags)

    file(GLOB_RECURSE MPACK_C_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mpack/src/mpack/*.c)
    add_library(mpack OBJECT ${MPACK_C_SOURCES})
    set_target_properties(mpack PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        C_VISIBILITY_PRESET hidden)

    target_include_directories(mpack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mpack/src/mpack)
    if(MPACK_DEBUG)
        target_compile_definitions(mpack PUBLIC MPACK_DEBUG=1 MPACK_BUFFER_SIZE=32)
    else()
        target_compile_definitions(mpack PUBLIC MPACK_STDIO=0 MPACK_WRITE_TRACKING=1 MPACK_BUFFER_SIZE=131072)
    endif()

    # DDTrace ZAI
    find_program(PHP_CONFIG php-config)
    execute_process(
            COMMAND ${PHP_CONFIG} --includes
            RESULT_VARIABLE PHP_CONFIG_INCLUDES_RESULT
            OUTPUT_VARIABLE PHP_CONFIG_INCLUDES
            ERROR_VARIABLE PHP_CONFIG_INCLUDES_ERR
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(ZAI_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/zend_abstract_interface")
    if("${PHP_CONFIG_INCLUDES_RESULT}" STREQUAL "0")
        string(REPLACE "-I " "-I" PHP_INCLUDES "${PHP_CONFIG_INCLUDES} -I${ZAI_INCLUDES}")
        string(REPLACE "-I" "-isystem" PHP_INCLUDES "${PHP_INCLUDES}")
        string(REPLACE " " ";" PHP_INCLUDES "${PHP_INCLUDES}")
        message(STATUS "Using PHP include flags: ${PHP_INCLUDES}")
    else()
        message(FATAL_ERROR "Error executing ${PHP_CONFIG} --includes: ${PHP_CONFIG_INCLUDES_ERR}")
    endif()

    add_library(Tea STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/src/error.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/src/exceptions.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/src/extension.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/src/frame.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/src/ini.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/src/io.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/src/sapi.c)

    target_include_directories(Tea
      PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/>
             $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ddtrace/tea/include>)
    target_compile_options(Tea PRIVATE ${PHP_INCLUDES})

    add_library(Tea::Tea ALIAS Tea)

    add_library(tea-libphp INTERFACE)
    target_compile_options(tea-libphp INTERFACE ${PHP_INCLUDES})
    add_library(Tea::Php ALIAS tea-libphp)

    set(ZAI_MODULES json env config)
    add_library(zai_zend_abstract_interface INTERFACE)
    target_include_directories(zai_zend_abstract_interface INTERFACE ${ZAI_INCLUDES})

    add_subdirectory(ddtrace/zend_abstract_interface/zai_string)
    set_target_properties(zai_string PROPERTIES POSITION_INDEPENDENT_CODE 1)
    target_compile_options(zai_string PRIVATE ${PHP_INCLUDES})

    add_subdirectory(ddtrace/zend_abstract_interface/zai_assert)
    foreach(module json env config)
        set(module_name "zai_${module}")
        add_subdirectory(ddtrace/zend_abstract_interface/${module})
        set_target_properties(${module_name} PROPERTIES POSITION_INDEPENDENT_CODE 1)
        target_compile_options(${module_name} PRIVATE ${PHP_INCLUDES})
    endforeach()
endif()

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG eb3220622e73a4889eee355ffa37972b3cac3df5)
FetchContent_MakeAvailable(spdlog)

set(ZLIB_VERSION v1.3)
FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG        ${ZLIB_VERSION}
)
FetchContent_MakeAvailable(zlib)
if(NOT(MSVC))
    set_property(TARGET zlibstatic PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_compile_definitions(zlibstatic PUBLIC ZLIB_CONST=1)
target_include_directories(zlibstatic INTERFACE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})

include(ExternalProject)
ExternalProject_Add(event_rules
    GIT_REPOSITORY https://github.com/DataDog/appsec-event-rules.git
    GIT_TAG 1.8.0
    GIT_SHALLOW ON
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "")

# vim: set et:
