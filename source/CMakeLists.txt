configure_file(config.hpp.in ${CMAKE_BINARY_DIR}/include/config.hpp)

# Include every source file in source directory
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.cpp *.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Sources" FILES ${SOURCES})

add_library(async_postgres SHARED ${SOURCES})

target_link_libraries(async_postgres PRIVATE
    gmod::common
    gmod::helpers
    PostgreSQL::PostgreSQL
)

if(WIN32)
    target_link_libraries(async_postgres PRIVATE ws2_32)
endif()

target_include_directories(async_postgres PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_BINARY_DIR}/include
)

set_gmod_suffix_prefix(async_postgres)

# Autoinstall
set(AUTOINSTALL "" CACHE PATH "Autoinstall path")
if(IS_DIRECTORY ${AUTOINSTALL})
    autoinstall(async_postgres ${AUTOINSTALL})
endif()
