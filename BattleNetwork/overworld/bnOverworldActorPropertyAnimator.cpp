#include "bnOverworldActorPropertyAnimator.h"

#include "../bnAudioResourceManager.h"

namespace Overworld {
  bool ActorPropertyAnimator::IsAnimating() {
    return animating;
  }

  bool ActorPropertyAnimator::IsAnimatingPosition() {
    return animatingPosition;
  }

  void ActorPropertyAnimator::ToggleAudio(bool enable) {
    audioEnabled = enable;
  }

  void ActorPropertyAnimator::Reset() {
    propertyStates.clear();
    totalTime = 0.0;
    animating = false;
    animatingPosition = false;
    animatingDirection = false;
  }

  void ActorPropertyAnimator::AddKeyFrame(ActorPropertyAnimator::KeyFrame keyframe) {
    if (animating) {
      Reset();
    }

    totalTime += keyframe.duration;

    for (auto& propertyStep : keyframe.propertySteps) {
      auto& propertyState = propertyStates[propertyStep.property];

      PropertyKeyFrame propertykeyFrame;
      propertykeyFrame.ease = propertyStep.ease;
      propertykeyFrame.value = propertyStep.value;
      propertykeyFrame.stringValue = propertyStep.stringValue;
      propertykeyFrame.timePoint = totalTime;

      propertyState.keyFrames.push_back(propertykeyFrame);
    }
  }

  static float getDefault(Actor& actor, ActorProperty property) {
    switch (property) {
    case ActorProperty::animation:
      return 0.0;
    case ActorProperty::x:
      return actor.getPosition().x;
    case ActorProperty::y:
      return actor.getPosition().y;
    case ActorProperty::z:
      return actor.GetElevation();
    case ActorProperty::scale_x:
      return actor.getScale().x;
    case ActorProperty::scale_y:
      return actor.getScale().y;
    case ActorProperty::rotation:
      return actor.getRotation();
    case ActorProperty::direction:
      return (float)actor.GetHeading();
    }

    return 0.0f;
  }

  static void setProperty(Actor& actor, ActorProperty property, float value) {
    switch (property) {
    case ActorProperty::animation:
      break;
    case ActorProperty::x:
      actor.setPosition(value, actor.getPosition().y);
      break;
    case ActorProperty::y:
      actor.setPosition(actor.getPosition().x, value);
      break;
    case ActorProperty::z:
      actor.SetElevation(value);
      break;
    case ActorProperty::scale_x:
      actor.setScale(value, actor.getScale().y);
      break;
    case ActorProperty::scale_y:
      actor.setScale(actor.getScale().x, value);
      break;
    case ActorProperty::rotation:
      actor.setRotation(value);
      break;
    case ActorProperty::direction:
      actor.Face((Direction)value); // direction will always animate as Ease::floor from this
      break;
    }
  }

  void ActorPropertyAnimator::UseKeyFrames(Actor& actor) {
    time = 0.0;
    animating = true;

    for (auto& [property, state] : propertyStates) {
      auto& firstFrame = state.keyFrames[0];

      if (firstFrame.timePoint != 0.0) {
        firstFrame.previousValue = getDefault(actor, property);
      }
      else {
        firstFrame.previousValue = firstFrame.value;
        firstFrame.previousStringValue = firstFrame.stringValue;
      }

      for (auto i = 1; i < state.keyFrames.size(); i++) {
        auto& keyFrame = state.keyFrames[i];
        auto& previousKeyFrame = state.keyFrames[i - 1];

        keyFrame.previousValue = previousKeyFrame.value;
        keyFrame.previousStringValue = previousKeyFrame.stringValue;
        keyFrame.previousTimePoint = previousKeyFrame.timePoint;
      }

      auto& lastFrame = state.keyFrames[state.keyFrames.size() - 1];

      switch (property) {
      case ActorProperty::x:
      case ActorProperty::y:
      case ActorProperty::z:
        animatingPosition = true;
        break;
      case ActorProperty::direction:
        animatingDirection = true;
        break;
      case ActorProperty::sound_effect:
        if (firstFrame.timePoint == 0.0) {
          auto clip = Audio().LoadFromFile(firstFrame.stringValue);
          Audio().Play(clip, AudioPriority::high);
        }
        break;
      case ActorProperty::sound_effect_loop:
        if (lastFrame.timePoint < totalTime && !lastFrame.stringValue.empty()) {
          // inject a KeyFrame to keep the last frame looping to the end of the animation
          ActorPropertyAnimator::PropertyKeyFrame injectedFrame;
          injectedFrame.previousTimePoint = lastFrame.timePoint;
          injectedFrame.previousStringValue = lastFrame.stringValue;
          injectedFrame.timePoint = totalTime;

          state.keyFrames.push_back(injectedFrame);
        }
        break;
      }
    }
  }

  static float interpolate(Ease ease, float a, float b, float progress) {
    float curve;

    progress = std::clamp(progress, 0.0f, 1.0f);

    // curves based on https://developer.mozilla.org/en-US/docs/Web/CSS/easing-function
    // not using bezier but hoping it's close enough
    switch (ease) {
    case Ease::linear:
      curve = progress;
      break;
    case Ease::in:
      curve = std::pow(progress, 1.7f);
      break;
    case Ease::out:
      curve = 2.0f * progress - std::pow(progress, 1.7f);
      break;
    case Ease::in_out:
      curve = (-2 * progress * progress * progress) + (3 * progress * progress);
      break;
    case Ease::floor:
      curve = std::floor(progress);
      break;
    }

    return (b - a) * curve + a;
  }

  void ActorPropertyAnimator::Update(Actor& actor, double elapsed) {
    if (!animating) {
      return;
    }

    auto previousPosition = actor.getPosition();

    std::vector<ActorProperty> removeList;

    for (auto& [property, state] : propertyStates) {
      auto& keyFrame = state.keyFrames[state.currentKeyFrame];
      bool swappedKeyFrame = false;

      // use the latest key frame
      while (keyFrame.timePoint < time && state.currentKeyFrame < state.keyFrames.size() - 1) {
        state.currentKeyFrame += 1;
        keyFrame = state.keyFrames[state.currentKeyFrame];
        swappedKeyFrame = true;
      }

      // use previous string, we should only use a string if the time point has been passed
      auto& activeStringValue = time < keyFrame.timePoint ? keyFrame.previousStringValue : keyFrame.stringValue;

      // update property
      switch (property) {
      case ActorProperty::animation: {

        if (!activeStringValue.empty() && !actor.IsPlayingCustomAnimation() || actor.GetAnimationString() != activeStringValue) {
          // force an idle direction to prevent override
          actor.Face(actor.GetHeading());
          actor.PlayAnimation(activeStringValue, true);
        }
        break;
      }
      case ActorProperty::sound_effect: {
        if (swappedKeyFrame || keyFrame.timePoint < time && !activeStringValue.empty()) {
          auto clip = Audio().LoadFromFile(activeStringValue);
          Audio().Play(clip, AudioPriority::high);
        }
        break;
      }
      case ActorProperty::sound_effect_loop: {
        if (!activeStringValue.empty()) {
          auto clip = Audio().LoadFromFile(activeStringValue);
          Audio().Play(clip, AudioPriority::high);
        }
        break;
      }
      default: {
        auto timeSinceLastFrame = time - keyFrame.previousTimePoint;
        auto timeBetweenFrames = keyFrame.timePoint - keyFrame.previousTimePoint;
        auto progress = timeBetweenFrames == 0.0 ? 1.0 : float(timeSinceLastFrame / timeBetweenFrames);

        auto interpolatedValue = interpolate(
          keyFrame.ease,
          keyFrame.previousValue,
          keyFrame.value,
          progress
        );

        setProperty(actor, property, interpolatedValue);
      }
      }

      // mark deletion for this tracker if it has completed
      if (time > keyFrame.timePoint) {
        removeList.push_back(property);
      }
    }

    for (auto property : removeList) {
      propertyStates.erase(property);
    }

    auto positionDifference = actor.getPosition() - previousPosition;

    if (animatingPosition && !actor.IsPlayingCustomAnimation()) {
      // resolve direction and player animation
      if (positionDifference.x != 0.0f || positionDifference.y != 0.0f) {
        float distance = std::sqrt(positionDifference.x * positionDifference.x + positionDifference.y * positionDifference.y);
        auto newHeading = animatingDirection ? actor.GetHeading() : Actor::MakeDirectionFromVector(positionDifference);

        if (distance <= actor.GetWalkSpeed() * float(elapsed)) {
          actor.Walk(newHeading, false); // Don't actually move or collide, but animate
        }
        else {
          actor.Run(newHeading, false);
        }
      }
      else {
        actor.Face(actor.GetHeading());
      }
    }

    time += elapsed;

    if (propertyStates.empty()) {
      Reset();
      if (onComplete) onComplete();
    }
  }
}
