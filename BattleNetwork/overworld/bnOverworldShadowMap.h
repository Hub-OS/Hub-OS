#pragma once
#include <vector>

namespace Overworld {
  class Map;

  class ShadowMap {
  public:
    ShadowMap(size_t cols, size_t rows);

    void CalculateShadows(Map& map);
    bool HasShadow(size_t x, size_t y, size_t layer);
  private:
    std::vector<size_t> shadows;
    size_t cols, rows;
  };
}
