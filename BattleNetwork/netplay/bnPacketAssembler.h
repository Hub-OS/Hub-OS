#pragma once

#include <Poco/Buffer.h>
#include <map>
#include <vector>
#include <algorithm>
#include <assert.h>
#include "../bnLogger.h"

class PacketAssembler {
public:
    static const size_t hash(size_t start, size_t len) { return 2 * (start + len); }

private:

  class range {
    size_t m_start{}, m_end{}, m_size{};

  public:
    range(size_t start, size_t end) {
      m_start = start;
      m_end = end;
      m_size = (end - start)+1;
    }

    const size_t start() const { return m_start; }
    const size_t end() const { return m_end; }
    const size_t size() const { return m_size; }
    const bool find(size_t elem) { return elem <= m_end && elem >= m_start; }
  };

  struct chunk {
    size_t id{};
    Poco::Buffer<char> data;
  };

  struct chunk_group {
    range m_range;
    std::vector<chunk> m_chunks;

    chunk_group(const range& range) : m_range(range) {}

    const size_t id() const { return m_range.start(); }

    const bool corrupt() const {
      return m_chunks.size() > m_range.size();
    }

    const bool full() const {
      return m_chunks.size() == m_range.size(); 
    }

    Poco::Buffer<char> assemble() {
      Poco::Buffer<char> data{ 0 };

      if (m_chunks.size() != m_range.size()) {
        Logger::Logf("Packet chunk assembly failed! Recieved: %d Expected: %d", m_chunks.size(), m_range.size());
        return data;
      }

      std::sort(m_chunks.begin(), m_chunks.end(), [](const chunk& first, const chunk& second) {
        return first.id < second.id;
        });

      for (auto& chunk : m_chunks) {
        data.append(chunk.data);
      }

      return data;
    }

    void insert(const chunk& chunk) {
      m_chunks.push_back(chunk);
    }
  };

  std::map<size_t, chunk_group> processing; //!< Key: hash, value: chunks 

public:
  std::vector<Poco::Buffer<char>> Assemble() {
    std::vector<Poco::Buffer<char>> packets;

    auto iter = processing.begin();

    while(iter != processing.end()) {
      auto& chunks = iter->second;

      bool full = chunks.full();
      bool corrupt = chunks.corrupt();

      if (full) {
        packets.push_back(chunks.assemble());
      }

      if (full || corrupt) {
        if (corrupt) {
          Logger::Logf("Dropping corrupt BigData packet %d", chunks.id());
        }
        iter = processing.erase(iter);
        continue;
      }

      iter++;
    }

    return packets;
  }

  void Process(size_t start, size_t end, size_t id, const Poco::Buffer<char> body) {
    size_t hash = PacketAssembler::hash(start, end - start + 1);

    auto iter = processing.find(hash);
    if (iter == processing.end()) {
      auto res = processing.insert(std::make_pair(hash, chunk_group(range(start, end))));

      iter = res.first;
    }

    iter->second.insert(chunk{ id, body });
  }
};