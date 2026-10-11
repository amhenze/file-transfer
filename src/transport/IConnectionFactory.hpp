#pragma once

#include "IConnection.hpp"
#include <memory>

class IConnectionFactory {
public:
  virtual ~IConnectionFactory() = default;
  virtual std::unique_ptr<IConnection> createConnection() = 0;
};
