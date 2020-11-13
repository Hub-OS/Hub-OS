#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnArtifact.h"
#include "bnSpriteProxyNode.h"

class Character;
class Target;

class MachGunCardAction : public CardAction {
  int damage{};
  SpriteProxyNode machgun;
  Animation machgunAnim;
  Entity* target{ nullptr };
  Battle::Tile* targetTile{ nullptr };
  bool moveUp{ true }, firstSpawn{ true };

  void FreeTarget();
  Battle::Tile* MoveRectical(Field*, bool columnMove);

public:
  MachGunCardAction(Character* owner, int damage);
  ~MachGunCardAction();

  void Execute() override;
  void EndAction() override;
  void OnAnimationEnd() override;
};

class Target : public Artifact {
  Animation anim;
  double attack{}; //!< in seconds
  int damage{};
public:
  Target(Field*, int damage);
  ~Target();

  void OnSpawn(Battle::Tile& start) override;
  void OnUpdate(float elapsed) override;
  void OnDelete() override;
};