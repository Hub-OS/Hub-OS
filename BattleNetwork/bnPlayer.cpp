#include "bnPlayer.h"
#include "bnNaviExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"
#include "bnAura.h"

#include "bnBubbleTrap.h"
#include "bnBubbleState.h"

#define RESOURCE_PATH "resources/navis/megaman/megaman.animation"

Player::Player()
  :
  state(PLAYER_IDLE),
  chargeComponent(this),
  AI<Player>(this),
  Character(Rank::_1)
{
  this->ChangeState<PlayerIdleState>();
  
  // The charge component is also a scene node
  // Make sure the charge is in front of this node
  // Otherwise children scene nodes are drawn behind 
  // their parents
  chargeComponent.SetLayer(-1);
  this->AddNode(&chargeComponent);
  chargeComponent.setPosition(0, -20.0f); // translate up -20

  health = 900;
  busterDamage = 1;
  chargedBusterDamage = 10;
  SetName("Megaman");
  SetLayer(0);
  team = Team::RED;

  hitCount = 0;

  setScale(2.0f, 2.0f);

  animationComponent = new AnimationComponent(this);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();
  this->RegisterComponent(animationComponent);

  setTexture(*TEXTURES.GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  previous = nullptr;

  playerControllerSlide = false;
}

Player::~Player() {
}

void Player::OnUpdate(float _elapsed) {
  if (GetTile() != nullptr) {
    setPosition(tileOffset.x + GetTile()->getPosition().x, tileOffset.y + GetTile()->getPosition().y);
  }

  if (GetFirstComponent<BubbleTrap>()) {
    this->ChangeState<BubbleState<Player>>();
  }

  AI<Player>::Update(_elapsed);

  //Components updates
  chargeComponent.Update(_elapsed);
}

void Player::Attack() {
  if (tile->GetX() <= static_cast<int>(field->GetWidth())) {
    auto damage = chargeComponent.IsFullyCharged() ? this->chargedBusterDamage : this->busterDamage;
    Spell* spell = new Buster(field, team, chargeComponent.IsFullyCharged(), damage);
    spell->SetDirection(Direction::RIGHT);
    field->AddEntity(*spell, tile->GetX(), tile->GetY());
  }
}

void Player::OnDelete() {
  chargeComponent.Hide();
  auto animationComponent = this->GetFirstComponent<AnimationComponent>();
  animationComponent->CancelCallbacks();
  animationComponent->SetAnimation(PLAYER_HIT);
  this->ChangeState<NaviExplodeState<Player>>(5, 0.65);
  this->LockState();
}

const float Player::GetHitHeight() const
{
  return 101.0f;
}

const bool Player::OnHit(const Hit::Properties props) {
  hitCount++;

  // Respond to the recoil bit state
  if ((props.flags & Hit::recoil) == Hit::recoil) {
    // When movement is interrupted because of a hit, we need to reset/flush the movement state data
    FinishMove();
    this->ChangeState<PlayerHitState>();
  }

  return true;
}

int Player::GetMoveCount() const
{
  return Entity::GetMoveCount();
}

int Player::GetHitCount() const
{
  return hitCount;
}

void Player::SetCharging(bool state)
{
  chargeComponent.SetCharging(state);
}

void Player::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;

  if (state == PLAYER_IDLE) {
    auto playback = Animator::Mode::Loop;
    animationComponent->SetAnimation(_state, playback);
  }
  else {
    animationComponent->SetAnimation(_state, 0, onFinish);
  }
}

void Player::EnablePlayerControllerSlideMovementBehavior(bool enable)
{
  playerControllerSlide = enable;
}

const bool Player::PlayerControllerSlideEnabled() const
{
  return playerControllerSlide;
}

void Player::SetBusterDamage(int damage)
{
  this->busterDamage = damage;
}

void Player::SetChargedBusterDamage(int damage)
{
  this->chargedBusterDamage = damage;
}
