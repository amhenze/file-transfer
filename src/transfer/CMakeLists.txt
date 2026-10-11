project(transfer)

add_library(${PROJECT_NAME} STATIC
    Transfer.cpp
    Protocol.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC transport storage common)

