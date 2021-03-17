#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnParticleImpact.h"
using sf::IntRect;

const std::string RESOURCE_PATH = "resources/spells/artifact_impact_fx.animation";
const std::string VULCAN_PATH   = "resources/spells/artifact_vulcan_impact.animation";
const std::string VOLCANO_PATH  = "resources/spells/volcano_hit.animation";
const std::string WIND_PATH     = "resources/spells/panel_wind.animation";

ParticleImpact::ParticleImpact(ParticleImpact::Type type) :
  type(type),
  randOffset(), 
  Artifact()
{
  SetLayer(-10);
  setTexture(Textures().GetTexture(TextureType::SPELL_IMPACT_FX));
  setScale(2.f, 2.f);

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  switch(type) {
  case Type::green:
    animation.SetAnimation("GREEN");
    break;
  case Type::yellow:
    animation.SetAnimation("YELLOW");
    break;
  case Type::blue:
    animation.SetAnimation("BLUE");
    break;
  case Type::thin:
    animation.SetAnimation("THIN");
    break;
  case Type::fire:
    animation.SetAnimation("FIRE");
    break;
  case Type::vulcan:
    animation = Animation(VULCAN_PATH);
    animation.SetAnimation("DEFAULT");
    setTexture(Textures().GetTexture(TextureType::SPELL_VULCAN_IMPACT_FX));
    break;
  case Type::volcano:
    animation = Animation(VOLCANO_PATH);
    animation.SetAnimation("HIT");
    setTexture(Textures().LoadTextureFromFile("resources/spells/volcano_hit.png"));
    break;
  case Type::wind:
    animation = Animation(WIND_PATH);
    animation.SetAnimation("DEFAULT");
    setTexture(Textures().LoadTextureFromFile("resources/spells/panel_wind.png"));
    break;
  default:
    animation.SetAnimation("GREEN");
  }

  auto onEnd = [this]() {
    Delete();
  };

  animation << onEnd;

  animation.Update(0, getSprite());

}

void ParticleImpact::OnSpawn(Battle::Tile& tile) {
  float height = GetHeight();
  float width = 10;

  switch (type) {
  case Type::vulcan:
  case Type::fire:
  case Type::volcano:
    // stay closer to the body
    height = GetHeight() / 2.0f;
    break;
  case Type::thin:
    width = 0; // TODO: make this an adjustable property like Set/Get Height
    break;
  case Type::wind:
    height = 0;
    width = 0;
    break;
  }

  randOffset = sf::Vector2f(float(rand() % static_cast<int>(width+1)), float(rand() % static_cast<int>(height+1)));
  randOffset.x *= rand() % 2 ? -1 : 1;
  randOffset.y = randOffset.y - GetHeight();
}

void ParticleImpact::SetOffset(const sf::Vector2f& offset)
{
  this->offset = offset;
}

void ParticleImpact::OnUpdate(double _elapsed) {
  animation.Update(_elapsed, getSprite());
  Entity::Update(_elapsed);

  setPosition(GetTile()->getPosition() + tileOffset + randOffset + offset);
}

void ParticleImpact::OnDelete()
{
  Remove();
}

ParticleImpact::~ParticleImpact()
{
}