#include "bnMysteryData.h"
#include "bnExplosion.h"
#include "bnTextureResourceManager.h"

MysteryData::MysteryData(Field* _field, Team _team) : Character() {
  setTexture(LOAD_TEXTURE(MISC_MYSTERY_DATA));
  setScale(2.f, 2.f);
  SetFloatShoe(true);

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/mobs/mystery_data/mystery_data.animation");
  animation->Reload();
  animation->SetAnimation("SPIN", Animator::Mode::Loop);
  SetHealth(1);
}

MysteryData::~MysteryData() {

}

void MysteryData::OnUpdate(float _elapsed) {
  if (tile == nullptr) return;

  if (GetHealth() == 0) {
    Delete();
    field->AddEntity(*new Explosion(field, team), *GetTile());
  }

  setPosition(tile->getPosition().x, tile->getPosition().y);
}

const bool MysteryData::OnHit(const Hit::Properties props) {
  if((props.flags & Hit::impact) == Hit::impact) {
    SetHealth(0);
    return true;
  }
  
  return false;
}

void MysteryData::RewardPlayer()
{
  animation->SetAnimation("GET");
}
