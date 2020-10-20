#pragma once
#include "bnUIComponent.h"
#include "bnAnimationComponent.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include <string>
#include <functional>

#include <SFML/Graphics.hpp>

enum class ActionLockoutType : unsigned {
  animation,
  async
};

enum class ActionLockoutGroup : unsigned {
  none,
  card,
  ability
};

struct ActionLockoutProperties {
  ActionLockoutType type;
  double cooldown;
  ActionLockoutGroup group;
};

// TODO: This was written before card actions had many sprite nodes involved (like hilt + sword piece)
// So the constructor, which was written to make card creation easy,
// has now made it convoluted and difficult. REWRITE THIS CHIP ACTION CONSTRUCTOR!
class CardAction : public Component, public SceneNode {
private:
  ActionLockoutProperties lockoutProps{};
  bool animationIsOver{ false };

protected:
  AnimationComponent* anim;
  std::string animation, nodeName;
  std::string uuid, prevState;
  SpriteProxyNode** attachment;
  std::function<void()> prepareActionDelegate;

protected:
  /*user defined */
  virtual void Execute() = 0;

  void AddAction(int frame, std::function<void()> action);

  void RecallPreviousState();

public:
  /** Override get owner to always return a character type */
  Character* GetOwner();

  CardAction() = delete;
  CardAction(const CardAction& rhs) = delete;
  CardAction(Character* owner, std::string animation, SpriteProxyNode** attachment, std::string nodeName);

  void OverrideAnimationFrames(std::list<OverrideFrame> frameData);

  virtual ~CardAction();

  void OnExecute();

  virtual void OnUpdate(float _elapsed);
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
  virtual void EndAction() = 0;
  virtual void OnAnimationEnd() = 0;

  void Inject(BattleSceneBase&) final;

  void SetLockout(const ActionLockoutProperties& props);
  void SetLockoutGroup(const ActionLockoutGroup& group);

  const ActionLockoutGroup GetLockoutGroup() const;
  const ActionLockoutType GetLockoutType() const;
  const bool IsAnimationOver() const;
  const bool IsLockoutOver() const;
};