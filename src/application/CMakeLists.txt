project(application)

add_library(${PROJECT_NAME} STATIC
    Client.cpp
    Server.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC transfer transport)

