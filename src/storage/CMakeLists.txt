project(storage)

add_library(${PROJECT_NAME} STATIC
    FileReader.cpp
    FileWriter.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC common)

