#include "bnOverworldShadowMap.h"

#include "bnOverworldMap.h"

#include <cstddef>

static inline size_t calculateIndex(size_t cols, size_t x, size_t y) {
  return y * cols + x;
}

namespace Overworld {
  ShadowMap::ShadowMap(size_t cols, size_t rows) :
    cols(cols),
    rows(rows)
  {
    shadows.resize(cols * rows, 0);
  }

  void ShadowMap::CalculateShadows(Map& map) {
    auto layerCount = map.GetLayerCount();

    for (size_t y = 0; y < rows; y++) {
      for (size_t x = 0; x < cols; x++) {
        auto shadowIndex = calculateIndex(cols, x, y);
        shadows[shadowIndex] = 0;

        for (auto index = layerCount - 1; index >= 1; index--) {
          auto& layer = map.GetLayer(index);
          auto tile = layer.GetTile((int)x, (int)y);

          auto tileMeta = map.GetTileMeta(tile->gid);

          // failed to load tile
          if (tileMeta == nullptr) {
            continue;
          }

          switch (tileMeta->shadow) {
          case TileShadow::automatic:
            // invisible tile
            if (tileMeta->type == TileType::invisible) {
              continue;
            }

            // hole
            if (map.IgnoreTileAbove(float(x), float(y), int(index - 1))) {
              continue;
            }
            break;
          case TileShadow::always:
            break;
          case TileShadow::never:
            continue;
          }

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
