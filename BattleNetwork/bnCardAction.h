#pragma once
#include "bnComponent.h"
#include "bnAnimationComponent.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include <string>
#include <functional>

#include <SFML/Graphics.hpp>

// TODO: This was written before card actions had many sprite nodes involved (like hilt + sword piece)
// So the constructor, which was written to make card creation easy,
// has now made it convoluted and difficult. REWRITE THIS CHIP ACTION CONSTRUCTOR!
class CardAction : public Component {
protected:
  AnimationComponent* anim;
  std::string animation, nodeName;
  std::string uuid, prevState;
  SpriteProxyNode** attachment;
  std::function<void()> prepareActionDelegate;

protected:
  /*user defined */
  virtual void Execute() = 0;

  void AddAction(int frame, std::function<void()> action)
  {
    anim->AddCallback(frame, action, Animator::NoCallback, true);
  }

  void RecallPreviousState() {
    if (anim) {
      anim->SetAnimation(prevState);
      anim->SetPlaybackMode(Animator::Mode::Loop);
    }
  }

public:
  /** Override get owner to always return a character type */
  Character* GetOwner() { return GetOwnerAs<Character>(); }

  CardAction() = delete;
  CardAction(const CardAction& rhs) = delete;

  CardAction(Character * owner, std::string animation, SpriteProxyNode** attachment, std::string nodeName)
    : Component(owner), animation(animation), nodeName(nodeName), attachment(attachment)
  {
    anim = owner->GetFirstComponent<AnimationComponent>();

    if (anim) {
      prepareActionDelegate = [this, owner, animation]() {
        prevState = anim->GetAnimationString();

        // use the current animation's arrangement, do not overload
        prevState = anim->GetAnimationString();;
        anim->SetAnimation(animation, [this]() {
          //Logger::Log("normal callback fired");
          RecallPreviousState();
          EndAction();
        });

        anim->OnUpdate(0);
        OnUpdate(0); // position to owner...

      };
    }
  }

  void OverrideAnimationFrames(std::list<OverrideFrame> frameData)
  {
    if (anim) {
      prepareActionDelegate = [this, frameData]() {
        prevState = anim->GetAnimationString();

        anim->OverrideAnimationFrames(animation, frameData, uuid);
        anim->SetAnimation(uuid, [this]() {
          //Logger::Log("custom callback fired");

          anim->SetPlaybackMode(Animator::Mode::Loop);
          RecallPreviousState();
          EndAction();
        });
        anim->OnUpdate(0);
        OnUpdate(0); // position to owner...
      };
    }
  }

  virtual ~CardAction()
  {
  }

  void OnExecute() {
    // prepare the animation behavior
    prepareActionDelegate();

    // run
    Execute();
  }

  virtual void OnUpdate(float _elapsed)
  {
    if (!GetOwner() || !GetOwner()->GetTile() || !attachment) return;

    // update node position in the animation
    auto baseOffset = anim->GetPoint(nodeName);
    auto origin = GetOwner()->getSprite().getOrigin();
    baseOffset = baseOffset - origin;

    (*attachment)->setPosition(baseOffset);
  }

  virtual void EndAction() = 0;

  void Inject(BattleScene &) final
  {
    // do nothing
  }
};