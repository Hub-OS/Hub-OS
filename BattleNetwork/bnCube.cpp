#include "bnCube.h"
#include "bnRockDebris.h"
#include "bnParticlePoof.h"
#include "bnTile.h"
#include "bnDefenseVirusBody.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

const int Cube::numOfAllowedCubesOnField = 2;

Cube::Cube(Field* _field, Team _team) : Obstacle(field, team), CounterTrait<Cube>(), pushedByDrag(false) {
  this->setTexture(LOAD_TEXTURE(MISC_CUBE));
  this->setScale(2.f, 2.f);
  this->SetFloatShoe(false);
  this->SetName("Cube");
  this->SetTeam(_team);

  this->SetHealth(200);
  this->timer = 100;

  whiteout = SHADERS.GetShader(ShaderType::WHITE);

  this->SetSlideTime(sf::seconds(1.0f / 5.0f)); // was 1/15 

  hit = false;

  this->previousDirection = Direction::NONE;

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);

  auto props = GetHitboxProperties();
  props.flags |= Hit::impact | Hit::breaking;
  props.damage = 200;
  this->SetHitboxProperties(props);
}

Cube::~Cube() {
}

bool Cube::CanMoveTo(Battle::Tile * next)
{
  if (next && next->IsWalkable()) {
    if (next->ContainsEntityType<Obstacle>()) {
      Entity* other = nullptr;

      bool stop = false;

      auto allEntities = next->FindEntities([&stop, &other, this](Entity* e) -> bool {
        Cube* isCube = dynamic_cast<Cube*>(other);

        if (isCube && isCube->GetElement() == Element::ICE && this->GetElement() == Element::ICE) {
          isCube->SlideToTile(true);
          Direction dir = this->GetDirection();
          isCube->Move(dir);
          stop = true;
        }
        else if (isCube) {
          stop = true;
        }

        return false;
      });

      if (stop) {
        this->SetDirection(Direction::NONE);
        this->previousDirection = Direction::NONE;
        pushedByDrag = false;
        return false;
      }
    }

    return true;
  }

  this->SetDirection(Direction::NONE);
  this->previousDirection = Direction::NONE;
  return false;
}

void Cube::OnUpdate(float _elapsed) {
  if (!!IsSliding()) {
    this->previousDirection = Direction::NONE;
  }

  if (CounterTrait<Cube>::GetCounterSize() > Cube::numOfAllowedCubesOnField) {
    if (this->IsLast()) {
      this->SetHealth(0); // Trigger death and deletion
    }
  }

  // May have just finished sliding
  this->tile->AffectEntities(this);

  // Keep momentum
  if (!IsSliding() && pushedByDrag && GetDirection() != Direction::NONE) {
    this->SlideToTile(true);
    this->Move(this->GetDirection());
    FinishMove();
  }


  if (timer <= 0 ) {
    this->SetHealth(0);
  }

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  timer -= _elapsed;
}

// Triggered by health == 0
void Cube::OnDelete() {
  this->RemoveDefenseRule(virusBody);
  delete virusBody;

  if (this->GetFirstComponent<AnimationComponent>()->GetAnimationString() != "APPEAR") {
    int intensity = rand() % 2;
    intensity += 1;

    auto left = (this->GetElement() == Element::ICE) ? RockDebris::Type::LEFT_ICE : RockDebris::Type::LEFT;
    this->GetField()->AddEntity(*new RockDebris(left, (double)intensity), *this->GetTile());


    intensity = rand() % 3;
    intensity += 1;
    auto right = (this->GetElement() == Element::ICE) ? RockDebris::Type::RIGHT_ICE : RockDebris::Type::RIGHT;
    this->GetField()->AddEntity(*new RockDebris(right, (double)intensity), *this->GetTile());

    auto poof = new ParticlePoof();
    GetField()->AddEntity(*poof, *GetTile());

    AUDIO.Play(AudioType::PANEL_CRACK);
  }

  tile->RemoveEntityByID(this->GetID());

  this->RemoveMeFromCounterList();

  this->Delete(); // TODO: shouldn't be necessary!
}

const float Cube::GetHeight() const
{
  return 64.0f;
}

const bool Cube::OnHit(const Hit::Properties props) {
  if (this->animation->GetAnimationString() == "APPEAR")
    return false;

  // breaking prop is insta-kill
  if ((props.flags & Hit::breaking) == Hit::breaking) {
    this->SetHealth(0);
    return true;
  }

  // Teams cannot accidentally pull cube into their side
  if(props.aggressor && (props.flags & Hit::drag) == Hit::drag){
    if(props.aggressor->GetTeam() == Team::RED) {
      if(props.drag == Direction::LEFT) {
        // take damage anyway, skipping the Character::ResolveBattleStatus() step
        this->SetHealth(this->GetHealth() - props.damage);

        // Do not resolve battle step damage or extra status information
        return false;
      }
    } else if(props.aggressor->GetTeam() == Team::BLUE) {
      if(props.drag == Direction::RIGHT) {
        // take damage anyway, skipping the Character::ResolveBattleStatus() step
        this->SetHealth(this->GetHealth() - props.damage);

        // Do not resolve battle step damage or extra status information
        return false;
      }
    }

    pushedByDrag = true;
  }

  AUDIO.Play(AudioType::HURT);
  
  return true;
}

void Cube::Attack(Character* other) {
  Obstacle* isObstacle = dynamic_cast<Obstacle*>(other);

  if (isObstacle) {
    // breaking prop is insta-kill
    auto props = isObstacle->GetHitboxProperties();
    if ((props.flags & Hit::breaking) == Hit::breaking) {
      return;
    }

    isObstacle->Hit(GetHitboxProperties());
    this->hit = true;
    return;
  }

  Character* isCharacter = dynamic_cast<Character*>(other);

  if (isCharacter && isCharacter != this) {
    this->SetHealth(0);
    auto props = GetHitboxProperties();

    Logger::Logf("cube hitbox damage is: %d", props.damage);

    isCharacter->Hit(GetHitboxProperties());
    this->hit = true;
  }
}

void Cube::SetAnimation(std::string animation)
{
  this->animation->SetAnimation(animation);
}

void Cube::OnSpawn(Battle::Tile & start)
{
  animation = new AnimationComponent(this);
  this->RegisterComponent(animation);
  animation->Setup("resources/mobs/cube/cube.animation");
  animation->Reload();

  animation->OnUpdate(0);

  auto onFinish = [this, &start]() {
    if (start.GetState() == TileState::ICE) {
      animation->SetAnimation("ICE");
      this->SetElement(Element::ICE);
    }
    else {
      animation->SetAnimation("NORMAL");
    }
  };

  if (start.IsReservedByCharacter() || start.ContainsEntityType<Character>()) {
    this->SetHealth(0);
    animation->SetAnimation("APPEAR", 0);
  }
  else {
    animation->SetAnimation("APPEAR", 0, onFinish);
  }

}
