#include <random>
#include <time.h>

#include "bnElecpulse.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSharedHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Elecpulse::Elecpulse(Team _team, int _damage) : Spell(_team) {
  SetLayer(0);
  SetPassthrough(true);
  SetElement(Element::elec);
  HighlightTile(Battle::Tile::Highlight::flash);
  setScale(2.f, 2.f);
  progress = 0.0f;

  damage = _damage;

  setTexture(Textures().GetTexture(TextureType::SPELL_ELEC_PULSE));

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/spells/elecpulse.animation");
  animation->Reload();

  auto onFinish = [this]() { Delete(); };
  animation->SetAnimation("PULSE", onFinish);
}

Elecpulse::~Elecpulse(void) {
}

void Elecpulse::OnSpawn(Battle::Tile & start)
{
  int step = 1;

  if (GetTeam() == Team::blue) {
    step = -1;
  }

  auto field = GetField();
  auto forward = GetField()->GetAt(start.GetX() + step, start.GetY());
  auto shared = new SharedHitbox(this);
  shared->HighlightTile(Battle::Tile::Highlight::flash);
  field->AddEntity(*shared, *forward);

  auto top = GetField()->GetAt(start.GetX() + step, start.GetY() - step);
  shared = new SharedHitbox(this);
  shared->HighlightTile(Battle::Tile::Highlight::flash);
  field->AddEntity(*shared, *top);

  auto bottom = GetField()->GetAt(start.GetX() + step, start.GetY()+ step);
  shared = new SharedHitbox(this);
  shared->HighlightTile(Battle::Tile::Highlight::flash);
  field->AddEntity(*shared, *bottom);
}

void Elecpulse::OnUpdate(double _elapsed) {
  GetTile()->AffectEntities(this);

  setPosition(tile->getPosition()+sf::Vector2f(70.0f, -60.0f));
}

void Elecpulse::OnDelete()
{
  Remove();
}

void Elecpulse::Attack(Character* _entity) {
    long ID = _entity->GetID();

    if (std::find(taggedCharacters.begin(), taggedCharacters.end(), ID) != taggedCharacters.end())
        return; // we've already hit this entity somewhere

    Hit::Properties props;
    props.element = GetElement();
    props.flags = Hit::recoil | Hit::stun | Hit::drag;
    props.drag = { (GetTeam() == Team::red) ? Direction::left : Direction::right, 1u };
    props.damage = damage;

    _entity->Hit(props);

    taggedCharacters.insert(taggedCharacters.begin(), _entity->GetID());
}

