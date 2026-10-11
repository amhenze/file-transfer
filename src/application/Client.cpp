#include "Client.hpp"
#include "transfer/Transfer.hpp"

#include <iostream>
#include <memory>
#include <string>

int Client::run(const std::filesystem::path &file,
                IConnectionFactory &connectionFactory) {
  try {
    auto connection = connectionFactory.createConnection();
    Transfer transfer(*connection);
    std::cout << "Transfer started: " << file << '\n';

    std::string error;
    if (transfer.sendFile(file, error)) {
      std::cout << "Transfer completed successfully\n";
      return 0;
    }

    std::cerr << "Transfer failed: " << error << '\n';
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Connection failed: " << e.what() << '\n';
    return 1;
  }
}
