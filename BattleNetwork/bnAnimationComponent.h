#pragma once
#include <string>
#include <assert.h>
#include <functional>

#include <iostream>

#include "bnAnimation.h"

using std::string;
using std::to_string;

class Entity;

// TODO: make component
class AnimationComponent {
public:
  AnimationComponent(Entity* _entity);
  ~AnimationComponent();

  void Update(float _elapsed);
  void Setup(string _path);
  void Reload();
  const std::string GetAnimationString() const;
  void SetPlaybackSpeed(const double playbackSpeed);
  void SetPlaybackMode(char playbackMode);
  void SetAnimation(string state, std::function<void()> onFinish = nullptr);
  void SetAnimation(string state, char playbackMode, std::function<void()> onFinish = std::function<void()>());
  void AddCallback(int frame, std::function<void()> onFrame, std::function<void()> outFrame = std::function<void()>(), bool doOnce = false);
  void CancelCallbacks();
private:
  Entity* entity;
  string entityName;
  string path;
  Animation animation;
  double speed;
};
