#include "bnSuperVulcan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

SuperVulcan::SuperVulcan(Team _team, int damage) : Spell(_team) {
  SetLayer(1);

  setTexture(Textures().GetTexture(TextureType::SPELL_SUPER_VULCAN));
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    Delete();
  };

  animation = Animation("resources/spells/spell_super_vulcan.animation");
  animation.Load();
  animation.SetAnimation("DEFAULT");
  animation << onFinish;
  animation.Update(0, getSprite());

  Audio().Play(AudioType::GUN, AudioPriority::highest);

  auto props = GetHitboxProperties();
  props.flags = props.flags & ~(Hit::flash | Hit::flinch);
  props.damage = damage;
  SetHitboxProperties(props);
}

SuperVulcan::~SuperVulcan() {
}

void SuperVulcan::OnUpdate(double _elapsed) {
  animation.Update(_elapsed, getSprite());
  tile->AffectEntities(*this);
}

void SuperVulcan::Attack(std::shared_ptr<Character> _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    Audio().Play(AudioType::HURT);
  }
}

void SuperVulcan::OnDelete()
{
  Remove();
}
