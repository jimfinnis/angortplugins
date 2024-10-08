cmake_minimum_required(VERSION 2.6)
project(ANGORTPLUGINS)

option(POSIXTHREADS "enable POSIX threading")

find_file(MAKEWORDS makeWords.pl PATHS /usr/local/share/angort 
    /usr/share/angort
    $ENV{HOME}/angort
    )
if(MAKEWORDS)
    message(STATUS "makeWords.pl found in ${MAKEWORDS}")
else()
    message(FATAL_ERROR "makeWords.pl cannot be located")
endif()

include("cmake/macros.cmake")

set(INSTALLDIR "$ENV{HOME}/.angort")
include_directories("$ENV{HOME}/include")

#####################################################################
#
# add the projects down here
#
#



addeasy(io sys array time csv regex serial noise colours catmullrom kbspline)

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
target_link_libraries(diamond ${DIAMOND_LIBRARIES} pthread)
target_include_directories(diamond PUBLIC ${DIAMOND_INCLUDE_DIRS})
endif(DIAMOND_LIBRARIES AND DIAMOND_INCLUDE_DIRS)

find_library(PNG_LIBRARIES png)
find_path(PNG_INCLUDE_DIRS png.h)
if(PNG_LIBRARIES AND PNG_INCLUDE_DIRS)
add(png png.cpp nvbdflib-1.0/nvbdflib.c) # include the font library
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
message("SDL2 ${SDL2_FOUND}")
message("SDL2_IMG ${SDL2_IMG_FOUND}")
message("SDL2_GFX ${SDL2_GFX_FOUND}")
message("SDL2_TTF ${SDL2_TTF_FOUND}")
if(SDL2_FOUND AND SDL2_IMG_FOUND AND SDL2_GFX_FOUND AND SDL2_TTF_FOUND)
addeasy(sdl)
target_link_libraries(sdl ${SDL2_LIBRARIES}
    ${SDL2_IMG_LIBRARIES} ${SDL2_GFX_LIBRARIES} ${SDL2_TTF_LIBRARIES})
file(GLOB sdlutils "sdl/utils/*.ang")
install(PROGRAMS ${sdlutils} DESTINATION "${INSTALLDIR}/sdlutils")
endif(SDL2_FOUND AND SDL2_IMG_FOUND AND SDL2_GFX_FOUND AND SDL2_TTF_FOUND)

find_package(Jack)
if(JACK_FOUND)
addeasy(midi)
target_link_libraries(midi ${JACK_LIBRARIES})
target_compile_options(midi PUBLIC ${JACK_CFLAGS})
target_include_directories(midi PUBLIC ${JACK_INCLUDE_DIRS})
endif(JACK_FOUND)

find_package(Curses)
if(CURSES_FOUND)
addeasy(curses)
target_link_libraries(curses ${CURSES_LIBRARIES})
target_compile_options(curses PUBLIC ${CURSES_CFLAGS})
target_include_directories(curses PUBLIC ${CURSES_INCLUDE_DIRS})
file(GLOB cursesutils "curses/utils/*.ang")
install(PROGRAMS ${cursesutils} DESTINATION "${INSTALLDIR}/cursesutils")
endif(CURSES_FOUND)

pkg_search_module(MPDCLIENT libmpdclient)
if(MPDCLIENT_FOUND)
addeasy(mpd)
target_link_libraries(mpd ${MPDCLIENT_LIBRARIES})
target_compile_options(mpd PUBLIC ${MPDCLIENT_CFLAGS})
target_include_directories(mpd PUBLIC ${MPDCLIENT_INCLUDE_DIRS})
endif(MPDCLIENT_FOUND)

pkg_search_module(TAGLIB taglib)
if(TAGLIB_FOUND)
addeasy(id3)
target_link_libraries(id3 ${TAGLIB_LIBRARIES})
target_compile_options(id3 PUBLIC ${TAGLIB_CFLAGS})
target_include_directories(id3 PUBLIC ${TAGLIB_INCLUDE_DIRS})
endif(TAGLIB_FOUND)

if(POSIXTHREADS)
    message("POSIX threads enabled")
    find_package(Threads)
endif()

pkg_search_module(YAML_CPP yaml-cpp)
if(YAML_CPP_FOUND)
addeasy(yaml)
target_link_libraries(yaml ${YAML_CPP_LIBRARIES})
target_compile_options(yaml PUBLIC ${YAML_CPP_CFLAGS})
target_include_directories(yaml PUBLIC ${YAML_CPP_INCLUDE_DIRS})
endif(YAML_CPP_FOUND)

pkg_search_module(OPENCV opencv)
if(OPENCV_FOUND)
addeasy(cv)
target_link_libraries(cv ${OPENCV_LIBRARIES})
target_compile_options(cv PUBLIC ${OPENCV_CFLAGS})
target_include_directories(cv PUBLIC ${OPENCV_INCLUDE_DIRS})
endif(OPENCV_FOUND)




##################################################################

# Installation target

install(PROGRAMS ${ANGSOFILES} DESTINATION ${INSTALLDIR})


