add_library(art_static STATIC
    art.cpp
    art.hpp
)
add_library(art_shared SHARED
    art.cpp
    art.hpp
)
target_include_directories(art_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(art_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})