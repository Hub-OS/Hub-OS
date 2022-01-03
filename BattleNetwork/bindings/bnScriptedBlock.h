#include <functional>
#include "../bnPlayerCustScene.h"
#include "../bnPlayer.h"

class ScriptedBlock : public PlayerCustScene::Piece {
  sol::state& script;

public:
  ScriptedBlock(sol::state& script);
  ~ScriptedBlock();
  void run(Player* player);
  std::function<void(Player*)> run_func;
};