#pragma once

#include "bnOverworldMap.h"
#include "bnAnimate.h"
#include "bnAnimation.h"

namespace Overworld {
  enum class NPCType :int {
    MR_PROG_UP = 0,
    MR_PROG_DOWN = 1,
    MR_PROG_LEFT = 2,
    MR_PROG_RIGHT = 3,
    MR_PROG_FIRE = 4,
    NUMBERMAN_DOWN,
    NUMBERMAN_DANCE
  };

  struct NPC {
    sf::Sprite sprite;
    NPCType type;
  };

  class InfiniteMap : public Overworld::Map
  {
  private:
    Overworld::Tile* head;
    int branchDepth;
    
    Animate animator;
    Animation progAnimations;
    Animation numbermanAnimations;

    std::vector<NPC*> npcs;

  public:
    InfiniteMap(int branchDepth, int numOfCols, int tileWidth, int tileHeight);
    ~InfiniteMap();

    // Overwrite
    virtual void DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const;
    virtual void Update(double elapsed);
  };
}
