#pragma once

#include "bnOverworldMap.h"
#include "bnOverworldSceneBase.h"
#include <optional>

namespace Overworld {
  std::optional<Map> LoadTiledMap(SceneBase& scene, const std::string& data);
}
