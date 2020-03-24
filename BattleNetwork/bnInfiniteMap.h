#pragma once

#include "bnOverworldMap.h"
#include "bnAnimator.h"
#include "bnAnimation.h"

namespace Overworld {
  /*! \brief Denotes which NPC to draw */
  enum class NPCType :int {
    MR_PROG_UP = 0,
    MR_PROG_DOWN = 1,
    MR_PROG_LEFT = 2,
    MR_PROG_RIGHT = 3,
    MR_PROG_FIRE = 4,
    NUMBERMAN_DOWN,
    NUMBERMAN_DANCE
  };

  /*! \brief Draw information for NPC */
  struct NPC {
    SpriteProxyNode sprite;
    NPCType type;
  };

  /*! \brief An infinitely generating overworld map
   * 
   * \warning This is hackey and not optimized and not good overworld code. 
   *          Not recommended to use for anything more than visual effect.
   * 
   * Uses the Overworld::Map draw steps to reduce code
   */
  class InfiniteMap : public Overworld::Map
  {
  private:
    Overworld::Tile* head; /*!< The head of the trail that tiles branch off of*/
    int branchDepth; /*!< 0 = no paths. 1 or more generates twists and turns for a bigger map */
    
    Animator animator; /*!< Animator object to use on any animation */
    Animation progAnimations; /*!< mr prog animations to animate */
    Animation numbermanAnimations; /*!< numberman animations to animate */

    std::vector<NPC*> npcs; /*!< list of npcs */

    public:
    /**
     * @brief Creates a path of length numOfCols and starts with an arrow at the top
     * 
     * Assigns the head pointer to the last tile in map
     */
    InfiniteMap(int branchDepth, int numOfCols, int tileWidth, int tileHeight);
    
    /**
     * @brief deconstructors
     */
    virtual ~InfiniteMap();

    /**
     * @brief Moves the tiles based on the camera's movement
     * @param target
     * @param states
     */
    virtual void DrawTiles(sf::RenderTarget& target, sf::RenderStates states) const;
    
    /**
     * @brief Randomly generates an NPC. If the branch depth isn't reach spawn paths.
     * @param elapsed
     */
    virtual void Update(double elapsed);
  };
}
