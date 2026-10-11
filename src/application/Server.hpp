#pragma once

#include "transport/IEventLoop.hpp"
#include "transport/IListener.hpp"

class Server {
public:
  int run(IListener &listener, IEventLoop &eventLoop);
};
