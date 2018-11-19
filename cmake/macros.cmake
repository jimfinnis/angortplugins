set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(PkgConfig REQUIRED)

set(ANGSOFILES "")

# first arg is name of plugin, which should be the name of the
# directory it is in. Second is words file which should
# be converted with makewords, remainder are other sources.
# Other libraries will need to be added separately.

macro(add name wordsfile)
    # command to add a file to be converted by makewords; it's
    # the second argument - the others are plain files.
    
    add_custom_command(
        OUTPUT ${wordsfile}.cpp
        DEPENDS ${name}/${wordsfile}
        COMMAND perl ${MAKEWORDS} ${CMAKE_SOURCE_DIR}/${name}/${wordsfile} > ${wordsfile}.cpp
    )
    
    # start the source list with that file.

    set(SOURCES ${wordsfile}.cpp)
    
    # append the other sources, prepending the directory name to
    # each one.
    
    foreach(f ${ARGN})
        set(SOURCES ${SOURCES} ${name}/${f})
    endforeach(f ${ARGN})
    
    # set up the target for a pos-independent shared lib

    add_library(${name} SHARED ${SOURCES})
#    target_link_libraries(${name} -langort)
    set_property(TARGET ${name} PROPERTY POSITION_INDEPENDENT_CODE ON)

    if(POSIXTHREADS)
        message("POSIX threads enabled")
        set_target_properties(${name} PROPERTIES COMPILE_DEFINITIONS ANGORT_POSIXLOCKS)
        target_compile_options(${name} PUBLIC "-pthread")
    endif(POSIXTHREADS)

    # the plugin dir name should also be in the include path

    target_include_directories(${name} PUBLIC ${name})

    # once built, copy libTARGET.so to TARGET.angso

    add_custom_command(
        TARGET ${name}
        POST_BUILD
        COMMAND cp $<TARGET_FILE:${name}> ${name}.angso
    )
    # add the angso file to a list we use for installation target
    set(ANGSOFILES ${ANGSOFILES} 
        ${CMAKE_BINARY_DIR}/${name}.angso)
endmacro(add)

# this handles the basic plugins, which are in directories of the same name and contain a single
# source file whose name is pluginname.cpp.


macro(addeasy)
    foreach(lib ${ARGV})
        add(${lib} ${lib}.cpp)
    endforeach(lib)
endmacro(addeasy)
    
