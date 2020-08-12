#include "bnLightning.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnHitbox.h"

#define RESOURCE_PATH "resources/spells/spell_lightning.animation"
#define RESOURCE_IMG  "resources/spells/spell_lightning.png"

Lightning::Lightning(Field* field, Team team) : Spell(field, team)
{
  setTexture(LOAD_TEXTURE_FILE(RESOURCE_IMG));
  anim = CreateComponent<AnimationComponent>(this);
  anim->SetPath(RESOURCE_PATH);
  anim->Load();
  anim->SetAnimation("DEFAULT", [this]() { this->Delete(); });
  setScale(2.f, 2.f);
  SetElement(Element::elec);
}

Lightning::~Lightning()
{
}

void Lightning::OnSpawn(Battle::Tile & start)
{
  
  int row = start.GetY();
  int col = start.GetX();

  for (int i = 1; i < 4; i++) {
    auto hitbox = new Hitbox(GetField(), GetTeam(), this->GetHitboxProperties().damage);
    hitbox->HighlightTile(Battle::Tile::Highlight::solid);
    hitbox->SetHitboxProperties(GetHitboxProperties());
    GetField()->AddEntity(*hitbox, col+i, row);
  }

  AUDIO.Play(AudioType::THUNDER);
}


void Lightning::OnUpdate(float _elapsed)
{ }

void Lightning::Attack(Character * _entity)
{ }

void Lightning::OnDelete()
{ 
  this->Remove();
}
