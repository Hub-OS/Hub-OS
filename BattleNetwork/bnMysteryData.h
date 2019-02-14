#pragma once
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;

/* Mystery data sits on the field and explodes if hit by anything */
/* Note: Must tie  together with BattleOverTrigger to turn into "GET" sprite*/
class MysteryData : public Character {
public:
  MysteryData(Field* _field, Team _team);
  virtual ~MysteryData(void);

  virtual void Update(float _elapsed);
  virtual const bool Hit(int damage, Hit::Properties props = Hit::DefaultProperties);

  void RewardPlayer();

  virtual bool Move(Direction _direction) { return false; }
  vector<Drawable*> GetMiscComponents() { return vector<Drawable*>(); }

protected:
  Texture* texture;
  AnimationComponent animation;
};
