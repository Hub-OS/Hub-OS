#pragma once
#include "bnAnimationComponent.h"
#include "bnCharacter.h"
#include "bnSpriteProxyNode.h"
#include <string>
#include <functional>

#include <SFML/Graphics.hpp>

class CardAction {
protected:
  Character* user;
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
  Character* GetUser() { return user; }

  CardAction() = delete;
  CardAction(const CardAction& rhs) = delete;

  CardAction(Character * user, std::string animation, SpriteProxyNode** attachment, std::string nodeName)
    : user(user), animation(animation), nodeName(nodeName), attachment(attachment)
  {
    anim = user->GetFirstComponent<AnimationComponent>();

    if (anim) {
      prepareActionDelegate = [this, user, animation]() {
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
    // update node position in the animation
    auto baseOffset = anim->GetPoint(nodeName);
    auto origin = GetUser()->getSprite().getOrigin();
    baseOffset = baseOffset - origin;

    (*attachment)->setPosition(baseOffset);
  }

  virtual void EndAction() = 0;
};