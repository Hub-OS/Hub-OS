#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <functional>
#include <assert.h>
#include <iostream>

struct Frame {
  float duration;
  sf::IntRect subregion;
  bool applyOrigin;
  sf::Vector2f origin;
  
  std::map<std::string, sf::Vector2f> points;
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
  std::map<int, std::function<void()>> queuedCallbacks; // used for adding new callbacks while updating
  std::map<int, std::function<void()>> queuedOnetimeCallbacks; 
  
  // Whichever frame list we're using, these are the available points
  std::map<std::string, sf::Vector2f> currentPoints;
  
  std::function<void()> onFinish;
  std::function<void()> queuedOnFinish;
  
  char playbackMode;
  
  bool isUpdating;
  bool callbacksAreValid;
  
  void UpdateCurrentPoints(int frameIndex, FrameList& sequence);
  
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
    
    On(const On& rhs) {
		//std::cout << "in cpy constructor" << std::endl;
		this->id = rhs.id;
		this->callback = rhs.callback;
		this->doOnce = rhs.doOnce;
	}
  };

  class Mode {
  private:
    int playback;
    
  public:

    friend class Animate;

    static const char NoEffect = 0x00;
    static const char Loop = 0x01;
    static const char Bounce = 0x02;
    static const char Reverse = 0x04;

    Mode(int playback) {
      this->playback = playback;
    }

    ~Mode() { ; }
  };

  Animate();
  Animate(Animate& rhs);
  ~Animate();

  char GetMode() { return playbackMode;  }
  
  const sf::Vector2f GetPoint(const std::string label) {
      if(currentPoints.find(label) == currentPoints.end()) {
          Logger::Log("Could not find point in current sequence named " + label);
          return sf::Vector2f();
      }
      return currentPoints[label];
  }
  
  void Clear() { 
	  callbacksAreValid = false;
	  queuedCallbacks.clear(); queuedOnetimeCallbacks.clear(); queuedOnFinish = nullptr;
	  nextLoopCallbacks.clear(); callbacks.clear(); onetimeCallbacks.clear(); onFinish = nullptr; playbackMode = 0; 
  }

  void operator() (float progress, sf::Sprite& target, FrameList& sequence);
  Animate& operator << (On rhs);
  Animate& operator << (char rhs);
  void operator << (std::function<void()> finishNotifier);

  void SetFrame(int frameIndex, sf::Sprite& target, FrameList& sequence) const;

};
