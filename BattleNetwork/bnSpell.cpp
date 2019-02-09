#include <random>
#include <time.h>

#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"


Spell::Spell(void) : animationComponent(this), Entity() {
  SetFloatShoe(true);
  SetLayer(1);
  hit = false;
  srand((unsigned int)time(nullptr));
  progress = 0.0f;
  hitHeight = 0.0f;
  direction = Direction::NONE;
  texture = nullptr;
  markTile = false;
}

Spell::~Spell(void) {
}

const bool Spell::IsTileHighlightEnabled() const {
  return markTile;
}

void Spell::EnableTileHighlight(bool enable)
{
  markTile = enable;
}
