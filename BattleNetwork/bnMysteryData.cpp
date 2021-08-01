#include "bnMysteryData.h"
#include "bnExplosion.h"
#include "bnField.h"
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

  auto impact = [this]() {
    SetHealth(0);
  };

  RegisterStatusCallback(Hit::impact, impact);
}

MysteryData::~MysteryData() {

}

void MysteryData::OnUpdate(double _elapsed) {
  if (tile == nullptr) return;

  if (GetHealth() == 0) {
    Delete();
    field->AddEntity(*new Explosion, *GetTile());
  }
}

void MysteryData::RewardPlayer()
{
  animation->SetAnimation("GET");
}
