#include "bnMysteryData.h"
#include "bnExplosion.h"
#include "bnTextureResourceManager.h"

MysteryData::MysteryData(Field* _field, Team _team) : animation(this), Character() {
  this->setTexture(LOAD_TEXTURE(MISC_MYSTERY_DATA));
  this->setScale(2.f, 2.f);
  this->SetFloatShoe(true);

  animation.Setup("resources/mobs/mystery_data/mystery_data.animation");
  animation.Reload();
  animation.SetAnimation("SPIN", Animate::Mode::Loop);
  this->SetHealth(1);
}

MysteryData::~MysteryData() {

}

void MysteryData::Update(float _elapsed) {
  if (tile == nullptr) return;

  if (this->GetHealth() == 0) {
    this->Delete();
    this->field->AddEntity(*new Explosion(this->field, this->team), this->GetTile()->GetX(), this->GetTile()->GetY());
  }

  animation.Update(_elapsed);
  setPosition(tile->getPosition().x, tile->getPosition().y);

  Character::Update(_elapsed);
}

const bool MysteryData::OnHit(const Hit::Properties props) {
  if((props.flags & Hit::impact) == Hit::impact) {
    this->SetHealth(0);
    return true;
  }
  
  return false;
}

void MysteryData::RewardPlayer()
{
  animation.SetAnimation("GET");
}
