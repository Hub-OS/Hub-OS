/*! \file bnBlockPackageManager.h */

#pragma once

#include <SFML/Graphics.hpp>
#include "bnPackageManager.h"
#include "bnElements.h"
#include "bnCard.h"
#include "bnPlayerCustScene.h"

/*! \brief Use this to register block mods with the engine
*/

struct BlockMeta final : public PackageManager<BlockMeta>::Meta<PlayerCustScene::Piece> {
  std::string name, description;
  std::vector<uint8_t> shape{};
  size_t color{};
  bool isProgram{};

  /**
  * @brief
  */
  BlockMeta();

  /**
   * @brief Delete
   */
  ~BlockMeta();

  void SetName(const std::string& name);
  void SetDescription(const std::string& desc);
  void SetColor(size_t colorIdx);
  void SetShape(std::vector<uint8_t> values);
  void AsProgram();
};

class BlockPackageManager : public PackageManager<BlockMeta> {};