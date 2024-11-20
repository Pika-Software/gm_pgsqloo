configure_file(config.hpp.in ${CMAKE_BINARY_DIR}/include/config.hpp)

# Include every source file in source directory
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.cpp *.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "Sources" FILES ${SOURCES})

add_library(pgsqloo SHARED ${SOURCES})

target_link_libraries(pgsqloo PRIVATE
    gmod::common
    gmod::helpers
    PostgreSQL::PostgreSQL
)

target_include_directories(pgsqloo PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_BINARY_DIR}/include
)

set_gmod_suffix_prefix(pgsqloo)

# Autoinstall
set(AUTOINSTALL "" CACHE PATH "Autoinstall path")
if(IS_DIRECTORY ${AUTOINSTALL})
    autoinstall(pgsqloo ${AUTOINSTALL})
endif()
