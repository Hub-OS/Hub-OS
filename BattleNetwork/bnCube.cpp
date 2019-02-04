#include "bnCube.h"
#include "bnRockDebris.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

Cube::Cube(Field* _field, Team _team) : animation(this), Character(Cube::Rank::_1) {
  this->setTexture(LOAD_TEXTURE(MISC_CUBE));
  this->setScale(2.f, 2.f);
  this->SetFloatShoe(false);
  this->SetName("Cube");
  this->SetTeam(_team);

  animation.Setup("resources/mobs/cube/cube.animation");
  animation.Reload();

  auto onfinish = [this]() { if (this->GetTile()->GetState() == TileState::ICE) { this->SetAnimation("ICE"); } else { this->SetAnimation("NORMAL");  } };

  animation.SetAnimation("APPEAR", 0, onfinish);

  this->SetHealth(200);
  this->timer = 100;

  animation.Update(0);

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
}

Cube::~Cube(void) {

}

bool Cube::CanMoveTo(Battle::Tile * next)
{
  return (Entity::CanMoveTo(next));
}

void Cube::Update(float _elapsed) {
  SetShader(nullptr);

  // May have completed a slide, affect tiles
  bool affected = false;
  Entity* target = nullptr;
  while (tile->GetNextEntity(target)) {
    Character* isCharacter = dynamic_cast<Character*>(target);
    if (isCharacter && isCharacter != this) {
      this->SetHealth(0);
      isCharacter->Hit(200);
      affected = true;
    }
  }

  // Keep momentum
  if (this->next == nullptr && this->GetPreviousDirection() != Direction::NONE) {
    if (!affected) {
      this->SlideToTile(true);
      this->Move(this->GetPreviousDirection());
    }
  }

  if (timer <= 0) {
    this->SetHealth(0);
  }

  animation.Update(_elapsed);
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  if (this->GetHealth() == 0) {
    double intensity = (double)(rand() % 5) + 1.0;
    this->GetField()->OwnEntity(new RockDebris(RockDebris::Type::LEFT, intensity), this->GetTile()->GetX(), this->GetTile()->GetY());
    this->GetField()->OwnEntity(new RockDebris(RockDebris::Type::RIGHT, intensity), this->GetTile()->GetX(), this->GetTile()->GetY());
    this->Delete();
    AUDIO.Play(AudioType::PANEL_CRACK);
  }

  Character::Update(_elapsed);

  timer -= _elapsed;
}

const bool Cube::Hit(int damage, Hit::Properties props) {
  if (this->animation.GetAnimationString() == "APPEAR")
    return false;

  int health = this->GetHealth() - damage;
  if (health <= 0) health = 0;

  this->SetHealth(health);

  SetShader(whiteout);

  AUDIO.Play(AudioType::HURT);
  
  return health;
}

void Cube::SetAnimation(std::string animation)
{
  this->animation.SetAnimation(animation);
}
