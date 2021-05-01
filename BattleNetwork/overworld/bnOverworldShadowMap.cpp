#include "bnOverworldShadowMap.h"

static inline size_t calculateIndex(size_t cols, size_t x, size_t y) {
  return y * cols + x;
}

namespace Overworld {
  void ShadowMap::CalculateShadows(Map& map) {
    cols = (size_t)map.GetCols();
    rows = (size_t)map.GetRows();

    shadows.resize(cols * rows);
    std::fill(shadows.begin(), shadows.end(), 0);


    for (size_t x = 0; x < cols; x++) {
      for (size_t y = 0; y < rows; y++) {
        for (auto index = map.GetLayerCount() - 1; index >= 1; index--) {
          auto& layer = map.GetLayer(index);
          auto& tile = layer.GetTile((int)x, (int)y);

          if (tile.gid == 0) {
            continue;
          }

          if (map.IgnoreTileAbove(float(x), float(y), int(index - 1))) {
            continue;
          }

          auto shadowIndex = calculateIndex(cols, x, y);
          shadows[shadowIndex] = index;
          break;
        }
      }
    }
  }

  bool ShadowMap::HasShadow(size_t x, size_t y, size_t layer) {
    if (x >= cols || y >= rows) {
      return false;
    }

    auto shadowIndex = calculateIndex(cols, x, y);

    if (shadowIndex > shadows.size()) {
      return false;
    }

    return layer < shadows[shadowIndex];
  }
}
