file(
    GLOB 
    SOURCES
    "*.cpp"
)

add_executable(
    ${PROJECT_NAME}
    ${SOURCES}
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/db
)