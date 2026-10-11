project(transport)

add_library(${PROJECT_NAME} STATIC
    TcpConnection.cpp
    TcpListener.cpp
    EpollEventLoop.cpp
)

