#pragma once
#include "bnOverworldSceneBase.h"

namespace Overworld {
  class OnlineArea : public SceneBase {
  private:
  public:

    /**
     * @brief Loads the player's library data and loads graphics
     */
    OnlineArea(swoosh::ActivityController&, bool guestAccount);

    /**
    * @brief deconstructor
    */
    ~OnlineArea();
  };
}