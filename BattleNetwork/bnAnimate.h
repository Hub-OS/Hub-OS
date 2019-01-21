#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <functional>
#include <assert.h>

struct Frame {
  float duration;
  sf::IntRect subregion;
  bool applyOrigin;
  sf::Vector2f origin;
};

class FrameList {
  std::vector<Frame> frames;
  float totalDuration;

public:
  friend class Animate;

  FrameList() { totalDuration = 0; }
  FrameList(const FrameList& rhs) { frames = rhs.frames; totalDuration = rhs.totalDuration; }

  void Add(float dur, sf::IntRect sub) {
    frames.push_back({ dur, sub, false, sf::Vector2f(0,0) });
    totalDuration += dur;
  }

  void Add(float dur, sf::IntRect sub, sf::Vector2f origin) {
    frames.push_back({ dur, sub, true, origin });
    totalDuration += dur;
  }

  const float GetTotalDuration() { return totalDuration; }

  const bool IsEmpty() const { return frames.empty(); }
};

class Animate {
private:
  std::map<int, std::function<void()>> callbacks;
  std::map<int, std::function<void()>> onetimeCallbacks;
  std::map<int, std::function<void()>> nextLoopCallbacks; // used to move over already called callbacks

  std::function<void()> onFinish;
  char playbackMode;
public:
  class On {
    int id;
    std::function<void()> callback;
    bool doOnce;

  public:
    friend class Animate;
    On(int id, std::function<void()> callback, bool doOnce = false) : id(id), callback(callback), doOnce(doOnce) {
      ;
    }
  };

  class Mode {
  private:
    int playback;
  public:

    friend class Animate;

    static const int Loop = 0x1;
    static const int Bounce = 0x1 << 1;
    static const int Reverse = 0x1 << 2;

    Mode(int playback) {
      this->playback = playback;
    }

    ~Mode() { ; }
  };

  Animate();
  Animate(Animate& rhs);
  ~Animate();

  void Clear() { callbacks.clear(); onetimeCallbacks.clear(); onFinish = nullptr; playbackMode = 0; }

  void operator() (float progress, sf::Sprite& target, FrameList& sequence);
  Animate& operator << (On rhs);
  Animate& operator << (Mode rhs);
  void operator << (std::function<void()> finishNotifier);

  void SetFrame(int frameIndex, sf::Sprite& target, FrameList& sequence) const;

};