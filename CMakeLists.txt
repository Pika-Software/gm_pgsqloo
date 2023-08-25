cmake_minimum_required(VERSION 3.22)

# Require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable -fPIC flag
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable IDE folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Force x86_64 architecture on MacOSX
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")

    # Disable deprecation warnings
    add_compile_options(
        -Wno-deprecated-declarations
    )
endif()

# Enable multithreaded compilation on Windows
# and force runtime to non debug
if(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/MP")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# Include extensions
add_subdirectory(cmake)

# Include libpqxx
if(WIN32)
    set(PostgreSQL_FOUND TRUE)
    SET(PostgreSQL_ROOT "${CMAKE_CURRENT_LIST_DIR}/libpq")
    set(PostgreSQL_INCLUDE_DIRS "${PostgreSQL_ROOT}/include")
    set(PostgreSQL_LIBRARIES 
        "${PostgreSQL_ROOT}/lib/win64/libpq.lib"
        "${PostgreSQL_ROOT}/lib/win64/libpgcommon.lib"
        "${PostgreSQL_ROOT}/lib/win64/libpgport.lib"
        "${PostgreSQL_ROOT}/lib/win64/libssl.lib"
        "${PostgreSQL_ROOT}/lib/win64/libcrypto.lib"
        "Secur32"
    )
endif()

add_subdirectory(third-party/libpqxx)
add_subdirectory(third-party/fmt EXCLUDE_FROM_ALL)

# Include garrysmod_common
find_garrysmod_common()
if(NOT GARRYSMOD_COMMON_FOUND) # Check if garrysmod_common has been found
    message(FATAL_ERROR "garrysmod_common not found")
endif()

# Get the latest abbreviated commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the current working branch
if(NOT DEFINED GIT_BRANCH)
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

project(gm_pgsqloo 
    VERSION 1.0.0
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/Pika-Software/gm_pgsqloo"
)

add_subdirectory(source)
