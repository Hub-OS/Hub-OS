#pragma once
#include "bnComponent.h"
#include "bnAnimationComponent.h"
#include "bnCharacter.h"
#include "bnSpriteSceneNode.h"
#include <string>
#include <functional>

#include <SFML/Graphics.hpp>

class ChipAction : public Component {
protected:
  AnimationComponent* anim;
  std::string animation, nodeName;
  std::string uuid, prevState;
  SpriteSceneNode** attachment;
  std::function<void()> prepareActionDelegate;

protected:
  /*user defined */
  virtual void Execute() = 0;

  void AddAction(int frame, std::function<void()> action)
  {
    anim->AddCallback(frame, action, std::function<void()>(), true);
  }

  void RecallPreviousState() {
    anim->SetAnimation(prevState);
    anim->SetPlaybackMode(Animator::Mode::Loop);
  }

public:
  /** Override get owner to always return a character type */
  Character* GetOwner() { return GetOwnerAs<Character>(); }

  ChipAction() = delete;
  ChipAction(const ChipAction& rhs) = delete;

  ChipAction(Character * owner, std::string animation, SpriteSceneNode** attachment, std::string nodeName)
    : Component(owner), animation(animation), nodeName(nodeName), attachment(attachment)
  {
    anim = owner->GetFirstComponent<AnimationComponent>();

    if (anim) {
      prevState = anim->GetAnimationString();

      prepareActionDelegate = [this, owner, animation]() {
        // use the current animation's arrangement, do not overload
        this->prevState = anim->GetAnimationString();;
        this->anim->SetAnimation(animation, [this]() {
          Logger::Log("normal callback fired");
          this->RecallPreviousState();
          this->EndAction();
        });

        anim->OnUpdate(0);
      };
    }
  }

  void OverrideAnimationFrames(std::list<OverrideFrame> frameData)
  {
    if (anim) {
      prepareActionDelegate = [this, frameData]() {
        anim->OverrideAnimationFrames(this->animation, frameData, this->uuid);
        anim->SetAnimation(this->uuid, [this]() {
          Logger::Log("custom callback fired");

          anim->SetPlaybackMode(Animator::Mode::Loop);
          this->RecallPreviousState();
          this->EndAction();
        });
        anim->OnUpdate(0);
        this->OnUpdate(0); // position to owner...
      };
    }
  }

  virtual ~ChipAction()
  {
  }

  void OnExecute() {
    // prepare the animation behavior
    prepareActionDelegate();

    // run
    this->Execute();
  }

  virtual void OnUpdate(float _elapsed)
  {
    if (!GetOwner() || !GetOwner()->GetTile() || !attachment) return;

    // update node position in the animation
    auto baseOffset = anim->GetPoint(nodeName);
    auto origin = GetOwner()->getOrigin();
    baseOffset = baseOffset - origin;

    (*attachment)->setPosition(baseOffset);
  }

  virtual void EndAction() = 0;

  void Inject(BattleScene &) final
  {
    // do nothing
  }
};