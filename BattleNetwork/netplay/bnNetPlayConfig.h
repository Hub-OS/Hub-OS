#pragma once

/*******************************
Prototype network stuff
********************************/
struct NetPlayConfig {
  static constexpr const std::size_t MAX_BUFFER_LEN = 1024;
  static constexpr const uint16_t OBN_PORT = 8765;

  // struct vars
  uint16_t myPort{ OBN_PORT };
  uint16_t remotePort{ OBN_PORT };
  std::string remoteIP{};
  uint16_t myNavi{ 0 };
};