#include "bnPlayer.h"
#include "bnNaviWhiteoutState.h"
#include "bnField.h"
#include "bnBusterCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnLogger.h"
#include "bnAura.h"

#include "bnBubbleTrap.h"
#include "bnBubbleState.h"

#define RESOURCE_PATH "resources/navis/megaman/megaman.animation"

Player::Player()
  :
  state(PLAYER_IDLE),
  chargeEffect(this),
  AI<Player>(this),
  formSize(0),
  Character(Rank::_1)
{
  ChangeState<PlayerIdleState>();
  
  // The charge component is also a scene node
  // Make sure the charge is in front of this node
  // Otherwise children scene nodes are drawn behind 
  // their parents
  chargeEffect.SetLayer(-2);
  AddNode(&chargeEffect);
  chargeEffect.setPosition(0, -20.0f); // translate up -20
  chargeEffect.EnableParentShader(false);

  SetLayer(0);
  team = Team::red;

  hitCount = 0;

  setScale(2.0f, 2.0f);

  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  previous = nullptr;
  playerControllerSlide = false;
  activeForm = nullptr;
  queuedAction = nullptr;
}

Player::~Player() {
}

void Player::OnUpdate(float _elapsed) {
  if (GetTile() != nullptr) {
    setPosition(tileOffset.x + GetTile()->getPosition().x, tileOffset.y + GetTile()->getPosition().y);
  }


  // TODO: is there a way to have custom states and respond to them?
  if (GetFirstComponent<BubbleTrap>()) {
    ChangeState<BubbleState<Player>>();
  }

  AI<Player>::Update(_elapsed);

  if (activeForm) {
    activeForm->OnUpdate(_elapsed, *this);
  }

  //Node updates
  chargeEffect.Update(_elapsed);
}

void Player::Attack() {
  // Queue an action for the controller to fire at the right frame
  // (MMBN has a specific pipeline that accepts input. This emulates that.)
  if (tile) {
    chargeEffect.IsFullyCharged() ? queuedAction = ExecuteChargedBusterAction() : queuedAction = ExecuteBusterAction();
  }
}

void Player::UseSpecial()
{
  if (tile) {
    queuedAction = ExecuteSpecialAction();
  }
}

void Player::OnDelete() {
  chargeEffect.Hide();
  auto animationComponent = GetFirstComponent<AnimationComponent>();

  if (animationComponent) {
      animationComponent->CancelCallbacks();
      animationComponent->SetAnimation(PLAYER_HIT);
  }

  ChangeState<NaviWhiteoutState<Player>>();
}

const float Player::GetHeight() const
{
  return 101.0f;
}

const bool Player::OnHit(const Hit::Properties props) {
  hitCount++;

  // Respond to the recoil bit state
  if ((props.flags & Hit::recoil) == Hit::recoil) {
    // When movement is interrupted because of a hit, we need to flush the movement state data
    FinishMove();
    ChangeState<PlayerHitState>();
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
  chargeEffect.SetCharging(state);
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

void Player::ActivateFormAt(int index)
{
  if (activeForm) {
    activeForm->OnDeactivate(*this);
    delete activeForm;
    activeForm = nullptr;
  }

  if (index >= 0 || index < forms.size()) {
    auto meta = forms[index];
    activeForm = meta->BuildForm();

    if (activeForm) {
      activeForm->OnActivate(*this);
    }
  }
}

void Player::DeactivateForm()
{
  if (activeForm) {
    activeForm->OnDeactivate(*this);
  }
}

const std::vector<PlayerFormMeta*> Player::GetForms()
{
  auto res = std::vector<PlayerFormMeta*>();

  for (int i = 0; i < formSize; i++) {
    res.push_back(forms[i]);
  }

  return res;
}

void Player::QueueAction(CardAction* action)
{
  if (queuedAction) {
    delete action;
    action = nullptr;
  }

  queuedAction = action;
}

bool Player::RegisterForm(PlayerFormMeta * info)
{
  if (formSize >= forms.size() || !info) return false;

  forms[formSize++] = info;
  return true;
}
