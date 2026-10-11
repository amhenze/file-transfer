#pragma once

#include "transport/IConnectionFactory.hpp"

#include <filesystem>

class Client {
public:
  int run(const std::filesystem::path &file,
          IConnectionFactory &connectionFactory);
};
