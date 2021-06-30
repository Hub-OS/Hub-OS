#pragma once

/*******************************
Prototype network stuff
********************************/
struct NetPlayConfig {
  static constexpr const std::size_t MAX_BUFFER_LEN = 10240;
  static constexpr const uint16_t OBN_PORT = 8765;

  // struct vars
  uint16_t myNavi{ 0 };
};