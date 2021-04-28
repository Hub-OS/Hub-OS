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

struct BusterActionDeleter {
  void operator()(BusterEvent& in) {
    if (in.action->CanExecute()) {
      delete in.action;
    }
  }
};

Player::Player() :
  state(PLAYER_IDLE),
  chargeEffect(this),
  AI<Player>(this),
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

  setScale(2.0f, 2.0f);

  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  previous = nullptr;
  playerControllerSlide = false;
  activeForm = nullptr;

  auto recoil = [this]() {
    ChangeState<PlayerHitState>();
  };

  this->RegisterStatusCallback(Hit::flinch, Callback<void()>{ recoil });

  using namespace std::placeholders;
  auto handler = std::bind(&Player::HandleBusterEvent, this, _1, _2);
  actionQueue.RegisterType<BusterEvent, BusterActionDeleter>(ActionTypes::buster, handler);

  CreateMoveAnimHash();

  // When we are actionable what should we be doing?
  actionQueue.SetActionableCallback([this] {
    SetAnimation("PLAYER_IDLE");
  });
}

Player::~Player() {
}

void Player::OnUpdate(double _elapsed) {
  if (GetTile() != nullptr) {
    setPosition(tileOffset.x + GetTile()->getPosition().x, tileOffset.y + GetTile()->getPosition().y);
  }

  AI<Player>::Update(_elapsed);

  if (activeForm) {
    activeForm->OnUpdate(_elapsed, *this);
  }

  //Node updates
  chargeEffect.Update(_elapsed);

  fullyCharged = chargeEffect.IsFullyCharged();
}

void Player::Attack() {
  CardAction* action = nullptr;

  // Queue an action for the controller to fire at the right frame
  if (tile) {
    fullyCharged? action = ExecuteChargedBuster() : action = ExecuteBuster(); 
    
    if (action) {
      action->PreventCounters();
      action->SetLockoutGroup(CardAction::LockoutGroup::weapon);
      BusterEvent event = { frames(0), frames(0), false, action };
      actionQueue.Add(std::move(event), ActionOrder::voluntary, ActionDiscardOp::until_eof);
    }
  }
}

void Player::UseSpecial()
{
  if (tile) {
    if (auto action = ExecuteSpecial()) {
      action->PreventCounters();
      action->SetLockoutGroup(CardAction::LockoutGroup::ability);
      BusterEvent event = { frames(0), frames(0), false, action };
      actionQueue.Add(std::move(event), ActionOrder::voluntary, ActionDiscardOp::until_eof);
    }
  }
}

void Player::HandleBusterEvent(const BusterEvent& event, const ActionQueue::ExecutionType& exec)
{
  Character::HandleCardEvent({ event.action }, exec);
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

int Player::GetMoveCount() const
{
  return Entity::GetMoveCount();
}

void Player::Charge(bool state)
{
  frame_time_t maxCharge = CalculateChargeTime(GetChargeLevel());
  if (activeForm) {
    maxCharge = activeForm->CalculateChargeTime(GetChargeLevel());
  }

  chargeEffect.SetMaxChargeTime(maxCharge);
  chargeEffect.SetCharging(state);
}

void Player::SetAttackLevel(unsigned lvl)
{
  stats.attack = std::min(PlayerStats::MAX_ATTACK_LEVEL, lvl);
}

const unsigned Player::GetAttackLevel()
{
  return stats.attack;
}

void Player::SetChargeLevel(unsigned lvl)
{
  stats.charge = std::min(PlayerStats::MAX_CHARGE_LEVEL, lvl);
}

const unsigned Player::GetChargeLevel()
{
  return stats.charge;
}


void Player::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;

  if (state == PLAYER_IDLE) {
    animationComponent->SetAnimation(_state, Animator::Mode::Loop);
  }
  else {
    animationComponent->SetAnimation(_state, 0, onFinish);
  }

  animationComponent->Refresh();
}

const std::string Player::GetMoveAnimHash()
{
  return moveAnimHash;
}

void Player::SlideWhenMoving(bool enable, const frame_time_t& frames)
{
  slideFrames = frames;
  playerControllerSlide = enable;
}

CardAction* Player::OnExecuteSpecialAction()
{
  if (specialOverride) {
    return specialOverride();
  }

  return nullptr;
}

frame_time_t Player::CalculateChargeTime(const unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * 1 - 100i
  * 2 - 90i
  * 3 - 80i
  * 4 - 70i
  * 5 - 60i
  */
  switch (chargeLevel) {
  case 1:
    return frames(90);
  case 2:
    return frames(80);
  case 3:
    return frames(70);
  case 4:
    return frames(60);
  }

  return frames(50);
}

CardAction* Player::ExecuteBuster()
{
   return OnExecuteBusterAction();
}

CardAction* Player::ExecuteChargedBuster()
{
   return OnExecuteChargedBusterAction();
}

CardAction* Player::ExecuteSpecial()
{
  return OnExecuteSpecialAction();
}

void Player::ActivateFormAt(int index)
{
  index = index - 1; // base 1 to base 0

  if (activeForm) {
    activeForm->OnDeactivate(*this);
    delete activeForm;
    activeForm = nullptr;
    RevertStats();
  }

  for (auto& node : GetChildNodes()) {
    node->AddTags({ Player::BASE_NODE_TAG });
  }

  if (index >= 0 || index < forms.size()) {
    auto meta = forms[index];
    activeForm = meta->BuildForm();

    if (activeForm) {
      SaveStats();
      activeForm->OnActivate(*this);
      CreateMoveAnimHash();
    }
  }

  // Find nodes that do not have tags, those are newly added
  for (auto& node : GetChildNodes()) {
    if (!node->HasTag(Player::BASE_NODE_TAG)) {
      // Tag them
      node->AddTags({ Player::FORM_NODE_TAG });
    }
  }
}

void Player::DeactivateForm()
{
  if (activeForm) {
    activeForm->OnDeactivate(*this);
    RevertStats();
    CreateMoveAnimHash();
  }
}

const bool Player::IsInForm() const
{
  return (this->activeForm != nullptr);
}

const std::vector<PlayerFormMeta*> Player::GetForms()
{
  auto res = std::vector<PlayerFormMeta*>();

  for (int i = 0; i < forms.size(); i++) {
    res.push_back(forms[i]);
  }

  return res;
}

ChargeEffectSceneNode& Player::GetChargeComponent()
{
  return chargeEffect;
}

void Player::OverrideSpecialAbility(const std::function<CardAction* ()>& func)
{
  specialOverride = func;
}

void Player::SaveStats()
{
  savedStats.element = GetElement();
  savedStats.charge = GetChargeLevel();
  savedStats.attack = GetAttackLevel();
}

void Player::RevertStats()
{
  SetChargeLevel(savedStats.charge);
  SetAttackLevel(savedStats.attack);
  SetElement(savedStats.element);
}

void Player::CreateMoveAnimHash()
{
  const auto i4_frames = frames(4);
  const auto i4_seconds = seconds_cast<float>(i4_frames);
  const auto i1_seconds = seconds_cast<float>(frames(1));

  this->moveStartupDelay = i4_frames;
  this->moveEndlagDelay = i4_frames;
  auto frame_data = std::initializer_list<OverrideFrame>{
    { 1, i4_seconds },
    { 2, i1_seconds },
    { 3, i1_seconds },
    { 4, i1_seconds },
    { 3, i1_seconds },
    { 2, i1_seconds },
    { 1, i4_seconds }
  };

  // creates and stores the new state in variable `moveAnimHash`
  animationComponent->OverrideAnimationFrames("PLAYER_MOVE", frame_data, moveAnimHash);
}

bool Player::RegisterForm(PlayerFormMeta * info)
{
  if (forms.size() >= Player::MAX_FORM_SIZE || !info) return false;
  forms.push_back(info);
  return true;
}
