#include "bnCube.h"
#include "bnRockDebris.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

const int Cube::numOfAllowedCubesOnField = 2;
int Cube::currCubeIndex = 0;
int Cube::cubesRemovedCount = 0;

Cube::Cube(Field* _field, Team _team) : animation(this), Obstacle(field, team) {
  this->setTexture(LOAD_TEXTURE(MISC_CUBE));
  this->setScale(2.f, 2.f);
  this->SetFloatShoe(false);
  this->SetName("Cube");
  this->SetTeam(_team);

  animation.Setup("resources/mobs/cube/cube.animation");
  animation.Reload();

  auto onfinish = [this]() { 
    if (this->GetTile()->GetState() == TileState::ICE) { 
      this->SetAnimation("ICE"); 
      this->SetElement(Element::ICE);
    } 
    else 
    { this->SetAnimation("NORMAL");  } 
  };

  animation.SetAnimation("APPEAR", 0, onfinish);

  this->SetHealth(200);
  this->timer = 100;

  animation.Update(0);

  whiteout = SHADERS.GetShader(ShaderType::WHITE);

  this->slideTime = sf::seconds(1.0f/15.0f);

  cubeIndex = ++currCubeIndex;

  hit = false;
}

Cube::~Cube(void) {
  ++cubesRemovedCount;
}

bool Cube::CanMoveTo(Battle::Tile * next)
{
  if (Entity::CanMoveTo(next)) {
    if (next->ContainsEntityType<Obstacle>()) {
      Entity* other = nullptr;

      bool stop = false;

      while (next->GetNextEntity(other)) {
        Cube* isCube = dynamic_cast<Cube*>(other);

        if (isCube && isCube->GetElement() == Element::ICE && this->GetElement() == Element::ICE) {
          isCube->SlideToTile(true);
          Direction dir = this->direction;
          std::cout << "Direction: " << (int)dir << std::endl;
          isCube->Move(dir);
          stop = true;
        }
        else if (isCube) {
          stop = true;
        }
      }

      if (stop) {
        this->SetDirection(Direction::NONE);
        this->previousDirection = Direction::NONE;
        return false;
      }
    }

    return true;
  }

  this->SetDirection(Direction::NONE);
  this->previousDirection = Direction::NONE;
  return false;
}

void Cube::Update(float _elapsed) {
  SetShader(nullptr);

  // May have just finished sliding
  this->tile->AffectEntities(this);

  // Keep momentum
  if (this->next == nullptr && this->GetPreviousDirection() != Direction::NONE) {
    if (!hit) {
      this->SlideToTile(true);
      this->Move(this->GetPreviousDirection());
    }
  }

  if (timer <= 0) {
    this->SetHealth(0);
  }

  animation.Update(_elapsed);
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  Character::Update(_elapsed);

  timer -= _elapsed;
}

void Cube::OnDelete() {
  double intensity = (double)(rand() % 2) + 1.0;

  auto left = (this->GetElement() != Element::ICE) ? RockDebris::Type::LEFT : RockDebris::Type::LEFT_ICE;
  auto right = (this->GetElement() != Element::ICE) ? RockDebris::Type::RIGHT : RockDebris::Type::RIGHT_ICE;

  this->GetField()->OwnEntity(new RockDebris(left, intensity), this->GetTile()->GetX(), this->GetTile()->GetY());
  this->GetField()->OwnEntity(new RockDebris(right, intensity), this->GetTile()->GetX(), this->GetTile()->GetY());
  this->Delete();
  AUDIO.Play(AudioType::PANEL_CRACK);
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

void Cube::Attack(Character* other) {
  Obstacle* isObstacle = dynamic_cast<Obstacle*>(other);

  if (isObstacle) {
    isObstacle->Hit(200);
    this->hit = true;
    return;
  }

  Character* isCharacter = dynamic_cast<Character*>(other);

  if (isCharacter && isCharacter != this) {
    this->SetHealth(0);
    isCharacter->Hit(200);
    this->hit = true;
  }
}

void Cube::SetAnimation(std::string animation)
{
  this->animation.SetAnimation(animation);
}
