project(file-transfer)

add_subdirectory(common)
add_subdirectory(storage)
add_subdirectory(transport)
add_subdirectory(transfer)
add_subdirectory(application)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE application transport)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

