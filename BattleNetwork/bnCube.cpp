#include "bnCube.h"
#include "bnRockDebris.h"
#include "bnParticlePoof.h"
#include "bnTile.h"
#include "bnDefenseVirusBody.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

const int Cube::numOfAllowedCubesOnField = 2;

Cube::Cube(Field* _field, Team _team) : Obstacle(field, team), InstanceCountingTrait<Cube>(), pushedByDrag(false) {
  setTexture(LOAD_TEXTURE(MISC_CUBE));
  setScale(2.f, 2.f);
  SetFloatShoe(false);
  SetName("Cube");
  SetTeam(_team);

  SetHealth(200);
  timer = 100;

  whiteout = SHADERS.GetShader(ShaderType::WHITE);

  SetSlideTime(sf::seconds(1.0f / 5.0f)); // 1/5 of 60 fps = 12 frames

  hit = false;

  previousDirection = Direction::none;

  virusBody = new DefenseVirusBody();
  AddDefenseRule(virusBody);

  auto props = GetHitboxProperties();
  props.flags |= Hit::impact | Hit::breaking;
  props.damage = 200;
  SetHitboxProperties(props);
}

Cube::~Cube() {
}

bool Cube::CanMoveTo(Battle::Tile * next)
{
  if (next && next->IsWalkable()) {
    if (next->ContainsEntityType<Obstacle>()) {
      bool stop = false;

      auto allEntities = next->FindEntities([&stop, this](Entity* e) -> bool {
        Cube* isCube = dynamic_cast<Cube*>(e);

        if (isCube && isCube->GetElement() == Element::ice && GetElement() == Element::ice) {
          isCube->SlideToTile(true);
          Direction dir = GetDirection();
          isCube->Move(dir);
          isCube->FinishMove();
          stop = true;
        }
        else if (isCube) {
          stop = true;
        }

        return false;
      });

      if (stop) {
        SetDirection(Direction::none);
        previousDirection = Direction::none;
        FinishMove();
        pushedByDrag = false;
        return false;
      }
    }

    return true;
  }

  SetDirection(Direction::none);
  previousDirection = Direction::none;
  return false;
}

void Cube::OnUpdate(float _elapsed) {
  if (!!IsSliding()) {
    previousDirection = Direction::none;
  }

  if (GetCounterSize() > Cube::numOfAllowedCubesOnField) {
    if (IsLast()) {
      SetHealth(0); // Trigger death and deletion
    }
  }

  // May have just finished sliding
  tile->AffectEntities(this);

  // Keep momentum
  if (!IsSliding() && pushedByDrag && GetDirection() != Direction::none) {
    SlideToTile(true);
    Move(GetDirection());
    FinishMove();
  }


  if (timer <= 0 ) {
    SetHealth(0);
  }

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  timer -= _elapsed;
}

// Triggered by health == 0
void Cube::OnDelete() {
  RemoveDefenseRule(virusBody);
  delete virusBody;

  if (GetFirstComponent<AnimationComponent>()->GetAnimationString() != "APPEAR") {
    int intensity = rand() % 2;
    intensity += 1;

    auto left = (GetElement() == Element::ice) ? RockDebris::Type::LEFT_ICE : RockDebris::Type::LEFT;
    GetField()->AddEntity(*new RockDebris(left, (double)intensity), *GetTile());


    intensity = rand() % 3;
    intensity += 1;
    auto right = (GetElement() == Element::ice) ? RockDebris::Type::RIGHT_ICE : RockDebris::Type::RIGHT;
    GetField()->AddEntity(*new RockDebris(right, (double)intensity), *GetTile());

    auto poof = new ParticlePoof();
    GetField()->AddEntity(*poof, *GetTile());

    Audio().Play(AudioType::PANEL_CRACK);
  }

  tile->RemoveEntityByID(GetID());

  RemoveInstanceFromCountedList();
  Remove();

}

const float Cube::GetHeight() const
{
  return 64.0f;
}

const bool Cube::OnHit(const Hit::Properties props) {
  if (animation->GetAnimationString() == "APPEAR")
    return false;

  // breaking prop is insta-kill
  if ((props.flags & Hit::breaking) == Hit::breaking) {
    SetHealth(0);
    return true;
  }

  // Teams cannot accidentally pull cube into their side
  if(props.aggressor && (props.flags & Hit::drag) == Hit::drag){
    if(props.aggressor->GetTeam() == Team::red) {
      if(props.drag == Direction::left) {
        // take damage anyway, skipping the Character::ResolveBattleStatus() step
        SetHealth(GetHealth() - props.damage);

        // Do not resolve battle step damage or extra status information
        return false;
      }
    } else if(props.aggressor->GetTeam() == Team::blue) {
      if(props.drag == Direction::right) {
        // take damage anyway, skipping the Character::ResolveBattleStatus() step
        SetHealth(GetHealth() - props.damage);

        // Do not resolve battle step damage or extra status information
        return false;
      }
    }

    pushedByDrag = true;
  }

  Audio().Play(AudioType::HURT);
  
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
    hit = true;
    return;
  }

  Character* isCharacter = dynamic_cast<Character*>(other);

  if (isCharacter && isCharacter != this) {
    SetHealth(0);
    auto props = GetHitboxProperties();
    isCharacter->Hit(GetHitboxProperties());
    hit = true;
  }
}

void Cube::SetAnimation(std::string animation)
{
  this->animation->SetAnimation(animation);
}

void Cube::OnSpawn(Battle::Tile & start)
{
  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath("resources/mobs/cube/cube.animation");
  animation->Reload();

  animation->OnUpdate(0);

  auto onFinish = [this, &start]() {
    if (start.GetState() == TileState::ice) {
      animation->SetAnimation("ICE");
      SetElement(Element::ice);
    }
    else {
      animation->SetAnimation("NORMAL");
    }
  };

  if (start.IsReservedByCharacter() || start.ContainsEntityType<Character>()) {
    SetHealth(0);
    animation->SetAnimation("APPEAR", 0);
  }
  else {
    animation->SetAnimation("APPEAR", 0, onFinish);
  }

}
