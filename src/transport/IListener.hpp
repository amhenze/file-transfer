#pragma once

#include "IConnection.hpp"
#include <memory>

class IListener {
public:
  virtual ~IListener() = default;
  virtual std::unique_ptr<IConnection> accept() = 0;
};
