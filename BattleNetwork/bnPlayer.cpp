#include "bnPlayer.h"
#include "bnNaviExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"
#include "bnAura.h"

#define RESOURCE_PATH "resources/navis/megaman/megaman.animation"

Player::Player(void)
  : health(500),
  state(PLAYER_IDLE),
  chargeComponent(this),
  animationComponent(this),
  AI<Player>(this),
  Character(Rank::_1)
{
  this->ChangeState<PlayerIdleState>();
  this->AddNode(&chargeComponent);

  name = "Megaman";
  SetLayer(0);
  team = Team::RED;

  moveCount = hitCount = 0;

  //Animation
  animationProgress = 0.0f;
  setScale(2.0f, 2.0f);

  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();

  textureType = TextureType::NAVI_MEGAMAN_ATLAS;
  setTexture(*TEXTURES.GetTexture(textureType));

  previous = nullptr;

  moveCount = 0;

  invincibilityCooldown = 0;
}

Player::~Player(void) {
}

void Player::Update(float _elapsed) {
  animationComponent.Update(_elapsed);

  if (_elapsed <= 0)
    return;

  if (tile != nullptr) {
    setPosition(tileOffset.x + tile->getPosition().x, tileOffset.y + tile->getPosition().y);
  }

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<NaviExplodeState<Player>>(5, 0.65);
    AI<Player>::Update(_elapsed);
    return;
  }

  if (invincibilityCooldown > 0) {
    if ((((int)(invincibilityCooldown * 15))) % 2 == 0) {
      this->Hide();
    }
    else {
      this->Reveal();
    }

    invincibilityCooldown -= _elapsed;
  }
  else {
    this->Reveal();
  }

  AI<Player>::Update(_elapsed);

  //Components updates
  chargeComponent.Update(_elapsed);

  Character::Update(_elapsed);
}

void Player::Attack(float _charge) {
  if (tile->GetX() <= static_cast<int>(field->GetWidth())) {
    Spell* spell = new Buster(field, team, chargeComponent.IsFullyCharged());
    spell->SetDirection(Direction::RIGHT);
    field->AddEntity(*spell, tile->GetX(), tile->GetY());
  }
}

void Player::SetHealth(int _health) {
  health = _health;

  if (health < 0) health = 0;
}

int Player::GetHealth() const {
  return health;
}

const bool Player::Hit(int _damage, Hit::Properties props) {
  if ((this->IsPassthrough() && (props.flags & Hit::pierce) != Hit::pierce) || invincibilityCooldown > 0) return false;

  if (health - _damage < 0) {
    health = 0;
  }
  else {
    health -= _damage;
    hitCount++;

    if ((props.flags & Hit::recoil) == Hit::recoil) {
      this->ChangeState<PlayerHitState, float>({ (float)props.secs });
    }
  }

  return true;
}

int Player::GetMoveCount() const
{
  return moveCount;
}

int Player::GetHitCount() const
{
  return hitCount;
}

AnimationComponent& Player::GetAnimationComponent() {
  return animationComponent;
}

void Player::SetCharging(bool state)
{
  chargeComponent.SetCharging(state);
}

void Player::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;

  if (state == PLAYER_IDLE) {
    int playback = Animate::Mode::Loop;
    animationComponent.SetAnimation(_state, playback, onFinish);
  }
  else {
    animationComponent.SetAnimation(_state, onFinish);
  }
}
