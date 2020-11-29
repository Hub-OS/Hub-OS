#pragma once
#include "bnOverworldSceneBase.h"

namespace Overworld {
  class Homepage : public SceneBase {
  private:
    bool scaledmap{ false };

  public:

    /**
     * @brief Loads the player's library data and loads graphics
     */
    Homepage(swoosh::ActivityController&, bool guestAccount);

    /**
    * @brief deconstructor
    */
    ~Homepage();
  };
}