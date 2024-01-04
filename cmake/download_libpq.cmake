function(download_libpq)
    if (NOT WIN32)
        message(FATAL_ERROR "libpq precompiled only for Windows")
        return()
    endif()

    if (NOT EXISTS "${CMAKE_BINARY_DIR}/libpq")
        message(STATUS "Downloading precompiled static libpq...")
        file(DOWNLOAD "https://github.com/dankmolot/postgres/releases/download/static-library/headers.zip"              "${CMAKE_BINARY_DIR}/libpq/headers.zip") 
        file(DOWNLOAD "https://github.com/dankmolot/postgres/releases/download/static-library/libpq-Win32-Debug.zip"    "${CMAKE_BINARY_DIR}/libpq/libpq-win32-debug.zip")
        file(DOWNLOAD "https://github.com/dankmolot/postgres/releases/download/static-library/libpq-Win32-Release.zip"  "${CMAKE_BINARY_DIR}/libpq/libpq-win32-release.zip")
        file(DOWNLOAD "https://github.com/dankmolot/postgres/releases/download/static-library/libpq-x64-Debug.zip"      "${CMAKE_BINARY_DIR}/libpq/libpq-x64-debug.zip")
        file(DOWNLOAD "https://github.com/dankmolot/postgres/releases/download/static-library/libpq-x64-Release.zip"    "${CMAKE_BINARY_DIR}/libpq/libpq-x64-release.zip")

        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/libpq/headers.zip" DESTINATION "${CMAKE_BINARY_DIR}/libpq/include")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/libpq/libpq-win32-debug.zip" DESTINATION "${CMAKE_BINARY_DIR}/libpq/lib/win32/debug")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/libpq/libpq-win32-release.zip" DESTINATION "${CMAKE_BINARY_DIR}/libpq/lib/win32/release")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/libpq/libpq-x64-debug.zip" DESTINATION "${CMAKE_BINARY_DIR}/libpq/lib/x64/debug")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/libpq/libpq-x64-release.zip" DESTINATION "${CMAKE_BINARY_DIR}/libpq/lib/x64/release")
    endif()

    if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
        SET(PLATFORM_NAME "x64")
    else()
        SET(PLATFORM_NAME "win32")
    endif()

    set(PostgreSQL_ROOT "${CMAKE_BINARY_DIR}/libpq")
    set(PostgreSQL_INCLUDE_DIR "${PostgreSQL_ROOT}/include" PARENT_SCOPE)
    set(PostgreSQL_LIBRARY_ROOT "${PostgreSQL_ROOT}/lib/${PLATFORM_NAME}/$<IF:$<CONFIG:Debug>,debug,release>")
    set(PostgreSQL_LIBRARY 
        ${PostgreSQL_LIBRARY_ROOT}/libpq.lib
        ${PostgreSQL_LIBRARY_ROOT}/libpgcommon.lib
        ${PostgreSQL_LIBRARY_ROOT}/libpgport.lib
        Secur32
    PARENT_SCOPE)
endfunction()
