#include <SFML/Graphics.hpp>
using sf::Sprite;
using sf::IntRect;

#include "bnAnimation.h"
#include "bnFileUtil.h"
#include "bnLogger.h"
#include "bnEntity.h"
#include <cmath>
#include <chrono>

Animation::Animation() : animator(), path("") {
  progress = 0;
}

Animation::Animation(const char* _path) : animator(), path(std::string(_path)) {
  Reload();
}

Animation::Animation(const string& _path) : animator(), path(_path) {
  Reload();
}

Animation::Animation(const Animation& rhs) {
  *this = rhs;
}

Animation & Animation::operator=(const Animation & rhs)
{
  animations = rhs.animations;
  animator = rhs.animator;
  currAnimation = rhs.currAnimation;
  path = rhs.path;
  progress = rhs.progress;
  return *this;
}

Animation::~Animation() {
}

void Animation::Reload() {
  int frameAnimationIndex = -1;
  vector<FrameList> frameLists;
  string currentState = "";
  float currentAnimationDuration = 0.0f;
  int currentWidth = 0;
  int currentHeight = 0;
  bool legacySupport = false;
  progress = 0;
  string data = FileUtil::Read(path);
  int endline = 0;
  do {
    endline = (int)data.find("\n");
    string line = data.substr(0, endline);

    // NOTE: Support older animation files until we upgrade completely...
    if (line.find("VERSION") != string::npos) {
      string version = ValueOf("VERSION", line);
      if (version == "1.0") legacySupport = true;

    }
    else if (line.find("animation") != string::npos) {
      if (!frameLists.empty()) {
        //std::cout << "animation total seconds: " << sf::seconds(currentAnimationDuration).asSeconds() << "\n";
        //std::cout << "animation name push " << currentState << endl;
        animations.insert(std::make_pair(currentState, frameLists.at(frameAnimationIndex)));
        currentAnimationDuration = 0.0f;
      }
      string state = ValueOf("state", line);
      currentState = state;

      std::transform(currentState.begin(), currentState.end(), currentState.begin(), ::toupper);

      if (legacySupport) {
        string width = ValueOf("width", line);
        string height = ValueOf("height", line);
        //std::cout << "animation name in " << currentState << endl;

        currentWidth = atoi(width.c_str());
        currentHeight = atoi(height.c_str());
      }

      frameLists.push_back(FrameList());
      frameAnimationIndex++;
    }
    else if (line.find("frame") != string::npos) {
      string duration = ValueOf("duration", line);
      float currentFrameDuration = (float)atof(duration.c_str());

      int currentStartx = 0;
      int currentStarty = 0;
      float originX = 0;
      float originY = 0;

      if (legacySupport) {
        string startx = ValueOf("startx", line);
        string starty = ValueOf("starty", line);

        currentStartx = atoi(startx.c_str());
        currentStarty = atoi(starty.c_str());
      }
      else {
        string x = ValueOf("x", line);
        string y = ValueOf("y", line);
        string w = ValueOf("w", line);
        string h = ValueOf("h", line);
        string ox = ValueOf("originx", line);
        string oy = ValueOf("originy", line);

        currentStartx = atoi(x.c_str());
        currentStarty = atoi(y.c_str());
        currentWidth = atoi(w.c_str());
        currentHeight = atoi(h.c_str());
        originX = (float)atoi(ox.c_str());
        originY = (float)atoi(oy.c_str());
      }

      currentAnimationDuration += currentFrameDuration;

      if (legacySupport) {
        frameLists.at(frameAnimationIndex).Add(currentFrameDuration, IntRect(currentStartx, currentStarty, currentWidth, currentHeight));
      }
      else {
        frameLists.at(frameAnimationIndex).Add(currentFrameDuration, IntRect(currentStartx, currentStarty, currentWidth, currentHeight), sf::Vector2f(originX, originY));
      }
    }
    else if (line.find("point") != string::npos) {
      string pointName = ValueOf("label", line);
      string xStr = ValueOf("x", line);
      string yStr = ValueOf("y", line);

      std::transform(pointName.begin(), pointName.end(), pointName.begin(), ::toupper);

      int x = atoi(xStr.c_str());
      int y = atoi(yStr.c_str());

      frameLists[frameAnimationIndex].SetPoint(pointName, x, y);
    }

    data = data.substr(endline + 1);
  } while (endline > -1);

  // One more addAnimation to do if file is good
  if (frameAnimationIndex >= 0) {
    animations.insert(std::make_pair(currentState, frameLists.at(frameAnimationIndex)));
  }
}

void Animation::Load()
{
  Reload();
}

string Animation::ValueOf(string _key, string _line) {
  int keyIndex = (int)_line.find(_key);
  // assert(keyIndex > -1 && "Key was not found in .animation file.");
  string s = _line.substr(keyIndex + _key.size() + 2);
  return s.substr(0, s.find("\""));
}

void Animation::Refresh(sf::Sprite& target) {
  Update(0, target);
}

void Animation::Update(float elapsed, sf::Sprite& target, double playbackSpeed) {
  progress += elapsed * (float)std::fabs(playbackSpeed);

  std::string stateNow = currAnimation;

  if (noAnim == false) {
    animator(progress, target, animations[currAnimation]);
  }
  else {
    // effectively hide
    target.setTextureRect(sf::IntRect(0, 0, 0, 0));
  }

  if(currAnimation != stateNow) {
    // it was changed during a callback
    // apply new state to target on same frame
    animator(0, target, animations[currAnimation]);
    progress = 0;
    
    if (interruptCallback) {
      interruptCallback();
      interruptCallback = nullptr;
    }
  }

  const float duration = animations[currAnimation].GetTotalDuration();

  if(duration <= 0.f) return;

  // Since we are manually keeping track of the progress, we must account for the animator's loop mode
  while (progress > duration && (animator.GetMode() & Animator::Mode::Loop) == Animator::Mode::Loop) {
    progress -= duration;
  }
}

void Animation::SyncTime(float newTime)
{
  progress = newTime;
}

void Animation::SetFrame(int frame, sf::Sprite& target)
{
  if(path.empty() || animations.empty() || animations.find(currAnimation) == animations.end()) return;

  auto size = animations[currAnimation].GetFrameCount();

  if (frame <= 0 || frame > size) {
    progress = 0.0f;
    animator.SetFrame(int(size), target, animations[currAnimation]);

  }
  else {
    animator.SetFrame(frame, target, animations[currAnimation]);
    progress = 0.0f;

    while (frame) {
      progress += animations[currAnimation].GetFrame(--frame).duration;
    }
  }
}

void Animation::SetAnimation(string state) {
   if (interruptCallback) {
     interruptCallback();
     interruptCallback = nullptr;
   }

   RemoveCallbacks();
   progress = 0.0f;

   std::transform(state.begin(), state.end(), state.begin(), ::toupper);

   auto pos = animations.find(state);

   noAnim = false; // presumptious reset

   if (pos == animations.end()) {
     Logger::Log("No animation found in file for " + state);
     noAnim = true;
   }

   // Even if we don't have this animation, switch to it anyway
   currAnimation = state;
}

void Animation::RemoveCallbacks()
{
  animator.Clear();
  interruptCallback = nullptr;
}

const std::string Animation::GetAnimationString() const
{
  return currAnimation;
}

FrameList & Animation::GetFrameList(std::string animation)
{
  std::transform(animation.begin(), animation.end(), animation.begin(), ::toupper);
  return animations[animation];
}

Animation & Animation::operator<<(const Animator::On& rhs)
{
  animator << rhs;
  return *this;
}

Animation & Animation::operator<<(char rhs)
{
  animator << rhs;
  return *this;
}

Animation& Animation::operator<<(const std::string& state) {
  SetAnimation(state);
  return *this;
}

void Animation::operator<<(const std::function<void()>& onFinish)
{
  animator << onFinish;
}

sf::Vector2f Animation::GetPoint(const std::string & pointName)
{
  auto point = pointName;
  std::transform(point.begin(), point.end(), point.begin(), ::toupper);

  auto res = animator.GetPoint(point);

  return res;
}

void Animation::OverrideAnimationFrames(const std::string& animation, std::list <OverrideFrame> data, std::string& uuid)
{
  auto currentAnimation = animation;
  std::transform(currentAnimation.begin(), currentAnimation.end(), currentAnimation.begin(), ::toupper);

  if (uuid.empty()) {
    uuid = animation + "@" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  }

  animations.emplace(uuid, std::move(animations[animation].MakeNewFromOverrideData(data)));
}

void Animation::SyncAnimation(Animation& other)
{
  other.progress = progress;
  other.currAnimation = currAnimation;
}

void Animation::SetInterruptCallback(const std::function<void()> onInterrupt)
{
  interruptCallback = onInterrupt;
}
