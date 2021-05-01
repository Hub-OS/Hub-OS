#pragma once
#include "bnOverworldMap.h"

namespace Overworld {
  class ShadowMap {
  public:
    void CalculateShadows(Map& map);
    bool HasShadow(size_t x, size_t y, size_t layer);
  private:
    std::vector<size_t> shadows;
    size_t cols, rows;
  };
}
