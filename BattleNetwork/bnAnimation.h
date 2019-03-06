#pragma once
#include <string>
#include <assert.h>
#include <functional>

#include <iostream>

#include "bnAnimate.h"

using std::string;
using std::to_string;

#define ANIMATION_EXTENSION ".animation"

class Animation {
public:
  Animation();
  Animation(const char* path);
  Animation(string _path);
  ~Animation();

  void Reload();
  void Update(float _elapsed, sf::Sprite& target, double playbackSpeed = 1.0);
  void SetFrame(int frame, sf::Sprite& target);
  void SetAnimation(string state);

  void RemoveCallbacks();

  const std::string GetAnimationString() const;

  FrameList& GetFrameList(std::string animation);

  Animation& operator<<(Animate::On rhs);
  Animation& operator<<(char rhs);
  Animation& operator<<(std::string state);

  void operator<<(std::function<void()> onFinish);

private:
  string ValueOf(string _key, string _line);
protected:
  Animate animator;

  string path;
  string currAnimation;
  float progress;
  std::map<int, sf::Sprite> textures;
  std::map<string, FrameList> animations;
};
