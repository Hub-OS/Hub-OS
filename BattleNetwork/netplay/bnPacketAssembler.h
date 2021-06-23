#pragma once

#include <Poco/Buffer.h>
#include <map>
#include <vector>
#include <algorithm>
#include <optional>
#include <assert.h>
#include "../bnLogger.h"

class PacketAssembler {
private:
  std::unordered_map<size_t, std::map<size_t, std::vector<char>>> processing; //!< Key: start, value: chunk map

public:
  std::optional<Poco::Buffer<char>> Process(size_t start, size_t end, size_t id, const Poco::Buffer<char>& body) {
    auto iter = processing.find(start);

    if (iter == processing.end()) {
      // create a new chunk map
      auto res = processing.emplace(start, std::map<size_t, std::vector<char>>());

      iter = res.first;
    }

    auto& chunkMap = iter->second;

    // store the latest chunk
    chunkMap.emplace(id, std::vector<char>(body.begin(), body.end()));

    auto totalChunks = end - start + 1;

    if (chunkMap.size() < totalChunks) {
      // not enough stored chunks
      return {};
    }

    Poco::Buffer<char> data{ 0 };

    // maps are sorted
    for (auto& [_, chunk] : chunkMap) {
      data.append(chunk.data(), chunk.size());
    }

    return data;
  }
};