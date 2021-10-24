#ifdef BN_MOD_SUPPORT
#include "bnScriptedPlayer.h"
#include "bnScriptedCardAction.h"
#include "bnScriptedPlayerForm.h"
#include "../bnCardAction.h"

ScriptedPlayer::ScriptedPlayer(sol::state& script) :
  script(script),
  Player()
{
  chargeEffect.setPosition(0, -30.0f);


  SetLayer(0);
  SetTeam(Team::red);

  script["player_init"](*this);

  animationComponent->Reload();
  FinishConstructor();
}

ScriptedPlayer::~ScriptedPlayer() {
}

void ScriptedPlayer::SetChargePosition(const float x, const float y)
{
  chargeEffect.setPosition({ x,y });
}

void ScriptedPlayer::SetFullyChargeColor(const sf::Color& color)
{
  chargeEffect.SetFullyChargedColor(color);
}

void ScriptedPlayer::SetHeight(const float height)
{
  this->height = height;
}

void ScriptedPlayer::SetAnimation(const std::string& path)
{
  animationComponent->SetPath(path);
}

const float ScriptedPlayer::GetHeight() const
{
  return height;
}

Animation& ScriptedPlayer::GetAnimationObject()
{
  return animationComponent->GetAnimationObject();
}

Battle::Tile* ScriptedPlayer::GetCurrentTile() const
{
  return GetTile();
}

CardAction* ScriptedPlayer::OnExecuteBusterAction()
{
  if (!on_buster_action) {
    return nullptr;
  }

  CardAction* result{ nullptr };

  sol::object obj = on_buster_action(this);

  if (obj.valid()) {
    if (obj.is<std::unique_ptr<CardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      result = ptr.release();
    }
    else if (obj.is<std::unique_ptr<ScriptedCardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<ScriptedCardAction>&>();
      result = ptr.release();
    }
    else {
      Logger::Log("Lua function \"create_normal_attack\" didn't return a CardAction.");
    }
  }

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::weapon);
  }

  return result;
}

CardAction* ScriptedPlayer::OnExecuteChargedBusterAction()
{
  if (!on_charged_action) {
    return nullptr;
  }

  CardAction* result = activeForm ? activeForm->OnChargedBusterAction(*this) : nullptr;
  if (result) return result;

  sol::object obj = on_charged_action(this);

  if (obj.valid()) {
    if (obj.is<std::unique_ptr<CardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      result = ptr.release();
    }
    else if (obj.is<std::unique_ptr<ScriptedCardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<ScriptedCardAction>&>();
      result = ptr.release();
    }
    else {
      Logger::Log("Lua function \"create_charged_attack\" didn't return a CardAction.");
    }
  }

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::weapon);
  }

  return result;
}

CardAction* ScriptedPlayer::OnExecuteSpecialAction()
{
  if (!on_special_action) {
    return nullptr;
  }

  CardAction* result = activeForm ? activeForm->OnSpecialAction(*this) : nullptr;
  if (result) return result;

  sol::object obj = on_special_action(this);

  if (obj.valid()) {
    if (obj.is<std::unique_ptr<CardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<CardAction>&>();
      result = ptr.release();
    }
    else if (obj.is<std::unique_ptr<ScriptedCardAction>>())
    {
      auto& ptr = obj.as<std::unique_ptr<ScriptedCardAction>&>();
      result = ptr.release();
    }
    else {
      Logger::Log("Lua function \"create_special_attack\" didn't return a CardAction.");
    }
  }

  if (result) {
    result->SetLockoutGroup(CardAction::LockoutGroup::ability);
  }

  return result;
}

void ScriptedPlayer::OnUpdate(double _elapsed)
{
  Player::OnUpdate(_elapsed);

  if (on_update_func) {
    on_update_func(*this, _elapsed);
  }
}

ScriptedPlayerFormMeta* ScriptedPlayer::CreateForm()
{
  return new ScriptedPlayerFormMeta(forms.size() + 1u);
}

void ScriptedPlayer::AddForm(ScriptedPlayerFormMeta* meta)
{
  if (auto form = Player::AddForm(meta)) {
    form->SetUIPath(meta->GetUIPath());
  }
}

AnimationComponent::SyncItem& ScriptedPlayer::CreateAnimSyncItem(Animation* anim, SpriteProxyNode* node, const std::string& point)
{
  AnimationComponent::SyncItem* item = new AnimationComponent::SyncItem();
  item->anim = anim;
  item->node = node;
  item->point = point;

  animationComponent->AddToSyncList(*item);
  AddNode(item->node);

  return *item;
}

void ScriptedPlayer::RemoveAnimSyncItem(const AnimationComponent::SyncItem& item)
{
  animationComponent->RemoveFromSyncList(item);
  RemoveNode(item.node);
}

#endif