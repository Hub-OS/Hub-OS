#include "bnGame.h"
#include "bnResourceHandle.h"
#include "bnPA.h"
#include "bnPlayerPackageManager.h"
#include "bnMobPackageManager.h"
#include "bnBackground.h"
#include "bnCardFolder.h"

class GameUtils : public ResourceHandle {
  Game& game;

  public:
    GameUtils(Game& game);
    void LaunchMobBattle(PlayerMeta&, MobMeta&, std::shared_ptr<Background>, PA&, CardFolder*);
};