#include "bnAlphaArm.h"
#include "bnDefenseIndestructable.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"

AlphaArm::AlphaArm(Field* _field, Team _team) : Obstacle(_field, _team), isMoving(false) {
  this->setScale(2.f, 2.f);
  this->SetFloatShoe(true);
  this->SetTeam(_team);
  this->SetDirection(Direction::LEFT);
  this->EnableTileHighlight(false);

  this->SetHealth(999);

  this->SetSlideTime(sf::seconds(0.1333f)); // 8 frames

  Hit::Properties props = Hit::DefaultProperties;
  props.flags |= Hit::recoil | Hit::breaking;
  props.damage = 120;
  this->SetHitboxProperties(props);

  AddDefenseRule(new DefenseIndestructable());

  shadow = new SpriteSceneNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  this->AddNode(shadow);

  totalElapsed = 0;
}

AlphaArm::~AlphaArm() {
  this->RemoveNode(shadow);
  delete shadow;
}

bool AlphaArm::CanMoveTo(Battle::Tile * next)
{
  return true;
}

void AlphaArm::OnUpdate(float _elapsed) {
  totalElapsed += _elapsed;
  float delta = std::sinf(totalElapsed+1);

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - GetHitHeight() - delta);

  shadow->setPosition(-13, -4 + delta/2.0f); // counter offset the shadow node

  // May have just finished moving
  this->tile->AffectEntities(this);

  if (isMoving) {
    // Keep moving
    if (!this->IsSliding()) {
      this->SlideToTile(true);
      this->Move(this->GetDirection());
    }
  }

}

void AlphaArm::OnDelete() {

}

const bool AlphaArm::OnHit(const Hit::Properties props) {
  return false;
}

void AlphaArm::Attack(Character* other) {
  Obstacle* isObstacle = dynamic_cast<Obstacle*>(other);

  if (isObstacle) {
    auto props = Hit::DefaultProperties;
    props.damage = 9999;
    isObstacle->Hit(props);
    this->hit = true;
    return;
  }

  Character* isCharacter = dynamic_cast<Character*>(other);

  if (isCharacter && isCharacter != this) {
    isCharacter->Hit(GetHitboxProperties());
  }
}

const float AlphaArm::GetHitHeight() const
{
  return 10;
}
