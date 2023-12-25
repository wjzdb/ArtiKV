add_library(art_static STATIC
    art.cpp
    art.hpp
)
target_include_directories(art_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})