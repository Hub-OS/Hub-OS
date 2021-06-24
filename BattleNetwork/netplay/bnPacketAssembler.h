#pragma once

#include <Poco/Buffer.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <optional>
#include "../bnLogger.h"

class PacketAssembler {
private:
  std::unordered_map<size_t, std::map<size_t, std::vector<char>>> processing; //!< Key: start, value: chunk map

public:
  std::optional<Poco::Buffer<char>> Process(size_t start, size_t end, size_t id, const Poco::Buffer<char>& body);
};