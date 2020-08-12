#include "bnAlphaElectricalCurrent.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnHitbox.h"

#define RESOURCE_PATH "resources/mobs/alpha/alpha.animation"

AlphaElectricCurrent::AlphaElectricCurrent(Field* field, Team team, int count) : countMax(count), count(0), Spell(field, team)
{
  setTexture(LOAD_TEXTURE(MOB_ALPHA_ATLAS));
  anim = CreateComponent<AnimationComponent>(this);
  anim->SetPath(RESOURCE_PATH);
  anim->Load();
  setScale(2.f, 2.f);
  SetHeight(58);

  auto props = Hit::DefaultProperties;
  props.flags |= Hit::flinch;
  props.damage = 100;
  props.element = Element::elec;
  SetHitboxProperties(props);
}

AlphaElectricCurrent::~AlphaElectricCurrent()
{
}

void AlphaElectricCurrent::OnSpawn(Battle::Tile & start)
{
  auto endTrigger = [this]() {
    count++;
  };

  anim->SetAnimation("ELECTRIC", endTrigger);
  anim->SetPlaybackMode(Animator::Mode::Loop);

  auto attackTopAndBottomRowTrigger = [this]() {
    for (int i = 1; i < 4; i++) {
      auto hitbox = new Hitbox(GetField(), GetTeam(), 100);
      hitbox->HighlightTile(Battle::Tile::Highlight::solid);
      hitbox->SetHitboxProperties(GetHitboxProperties());
      GetField()->AddEntity(*hitbox, i, 1);
    }
    for (int i = 1; i < 4; i++) {
      auto hitbox = new Hitbox(GetField(), GetTeam(), 100);
      hitbox->HighlightTile(Battle::Tile::Highlight::solid);
      hitbox->SetHitboxProperties(GetHitboxProperties());
      GetField()->AddEntity(*hitbox, i, 3);
    }

    // This is the center tile that the electric current attack "appears" to be hovering over
    auto hitbox = new Hitbox(GetField(), GetTeam(), 100);
    hitbox->HighlightTile(Battle::Tile::Highlight::solid);
    hitbox->SetHitboxProperties(GetHitboxProperties());
    GetField()->AddEntity(*hitbox, 3, 2);

    AUDIO.Play(AudioType::THUNDER);
  };

  auto attackMiddleRowTrigger = [this]() {
    for (int i = 1; i < 4; i++) {
      auto hitbox = new Hitbox(GetField(), GetTeam());
      hitbox->SetHitboxProperties(GetHitboxProperties());
      hitbox->HighlightTile(Battle::Tile::Highlight::solid);
      GetField()->AddEntity(*hitbox, i, 2);
    }

    AUDIO.Play(AudioType::THUNDER);
  };


  anim->AddCallback(1, attackTopAndBottomRowTrigger);
  anim->AddCallback(4, attackMiddleRowTrigger);
}


void AlphaElectricCurrent::OnUpdate(float _elapsed)
{
  // In order to lay ontop of alpha's layer, we keep the spell on top of his position
  // but offset the sprite to be 2 tiles to the left...
  setPosition(tile->getPosition().x + tileOffset.x - (tile->GetWidth()*2.0f), tile->getPosition().y + tileOffset.y - GetHeight());

  if (count >= countMax) {
    Remove();
  }
}

void AlphaElectricCurrent::Attack(Character * _entity)
{
}

void AlphaElectricCurrent::OnDelete()
{
}
