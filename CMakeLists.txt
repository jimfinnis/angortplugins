cmake_minimum_required(VERSION 2.6)
project(ANGORTPLUGINS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(PkgConfig REQUIRED)

# first arg is name of plugin, which should be the name of the
# directory it is in. Second is words file which should
# be converted with makewords, remainder are other sources.
# Other libraries will need to be added separately.

# remaining args
macro(add name wordsfile)
    # command to add a file to be converted by makewords; it's
    # the second argument - the others are plain files.
    
    add_custom_command(
        OUTPUT ${wordsfile}.cpp
        DEPENDS ${name}/${wordsfile}
        COMMAND perl /home/white/angort/makeWords.pl ${CMAKE_SOURCE_DIR}/${name}/${wordsfile} > ${wordsfile}.cpp
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

    # the plugin dir name should also be in the include path

    target_include_directories(${name} PUBLIC ${name})

    # once built, copy libTARGET.so to TARGET.angso

    add_custom_command(
        TARGET ${name}
        POST_BUILD
        COMMAND cp $<TARGET_FILE:${name}> ${name}.angso
    )
endmacro(add)

# this handles the basic plugins, which are in directories of the same name and contain a single
# source file whose name is pluginname.cpp.


macro(addeasy)
    foreach(lib ${ARGV})
        add(${lib} ${lib}.cpp)
    endforeach(lib)
endmacro(addeasy)
    
addeasy(io sys array time csv regex serial noise
    #mpc
)

add(example_complex complex.cpp)
add(example_hello hello.cpp)

addeasy(vector2d)
target_link_libraries(vector2d m)

addeasy(json)
target_include_directories(json PUBLIC json/include)

add(udp udp.cpp udpclient.cpp udpserver.cpp)

# for Diamond Apparatus, this is just a library
find_library(DIAMOND_LIBRARIES diamondapparatus)
find_path(DIAMOND_INCLUDE_DIRS diamondapparatus/diamondapparatus.h)
if(DIAMOND_LIBRARIES AND DIAMOND_INCLUDE_DIRS)
addeasy(diamond)
target_link_libraries(diamond ${DIAMOND_LIBRARIES})
target_include_directories(diamond PUBLIC ${DIAMOND_INCLUDE_DIRS})
endif(DIAMOND_LIBRARIES AND DIAMOND_INCLUDE_DIRS)

find_library(PNG_LIBRARIES png)
find_path(PNG_INCLUDE_DIRS png.h)
if(PNG_LIBRARIES AND PNG_INCLUDE_DIRS)
addeasy(png)
target_link_libraries(png ${PNG_LIBRARIES})
target_include_directories(png PUBLIC ${PNG_INCLUDE_DIRS})
endif(PNG_LIBRARIES AND PNG_INCLUDE_DIRS)

find_library(CURL_LIBRARIES curl)
find_path(CURL_INCLUDE_DIRS curl/curl.h)
if(CURL_LIBRARIES AND CURL_INCLUDE_DIRS)
addeasy(curl)
target_link_libraries(curl ${CURL_LIBRARIES})
target_include_directories(curl PUBLIC ${CURL_INCLUDE_DIRS})
endif(CURL_LIBRARIES AND CURL_INCLUDE_DIRS)

pkg_search_module(LIBLO liblo)
if(LIBLO_FOUND)
addeasy(osc)
target_link_libraries(osc ${LIBLO_LIBRARIES})
target_include_directories(osc PUBLIC ${LIBLO_INCLUDE_DIRS})
endif(LIBLO_FOUND)

pkg_search_module(SDL2 sdl2)
pkg_search_module(SDL2_IMG SDL2_image)
pkg_search_module(SDL2_GFX SDL2_gfx)
pkg_search_module(SDL2_TTF SDL2_ttf)
if(SDL2_FOUND AND SDL2_IMG_FOUND AND SDL2_GFX_FOUND AND SDL2_TTF_FOUND)
addeasy(sdl)
target_link_libraries(sdl ${SDL2_LIBRARIES}
    ${SDL2_IMG_LIBRARIES} ${SDL2_GFX_LIBRARIES} ${SDL2_TTF_LIBRARIES})
endif(SDL2_FOUND AND SDL2_IMG_FOUND AND SDL2_GFX_FOUND AND SDL2_TTF_FOUND)

find_package(Jack)
if(JACK_FOUND)
addeasy(midi)
target_link_libraries(midi ${JACK_LIBRARIES})
target_include_directories(midi PUBLIC ${JACK_INCLUDE_DIRS})
endif(JACK_FOUND)

find_package(Curses)
if(CURSES_FOUND)
addeasy(curses)
target_link_libraries(curses ${CURSES_LIBRARIES})
target_include_directories(curses PUBLIC ${CURSES_INCLUDE_DIRS})
endif(CURSES_FOUND)

pkg_search_module(MPDCLIENT libmpdclient)
if(MPDCLIENT_FOUND)
addeasy(mpdclient)
