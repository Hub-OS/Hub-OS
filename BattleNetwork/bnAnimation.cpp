#include <SFML/Graphics.hpp>
using sf::Sprite;
using sf::IntRect;

#include "bnAnimation.h"
#include "bnFileUtil.h"
#include "bnLogger.h"
#include "bnEntity.h"
#include <cmath>

Animation::Animation() : animator(), path("") {
  ;
}

Animation::Animation(const char* _path) : animator(), path(std::string(_path)) {
    
}

Animation::Animation(string _path) : animator(), path(_path) {
}

Animation::~Animation() {
}

void Animation::Load() {
  int frameAnimationIndex = -1;
  vector<FrameList> frameLists;
  string currentState = "";
  float currentAnimationDuration = 0.0f;
  int currentWidth = 0;
  int currentHeight = 0;
  bool legacySupport = true;

  string data = FileUtil::Read(path);

  //Logger::Log("anim: path is: " + path);
  //Logger::Log("anim: data is: " + data);

  int endline = 0;
  do {
    endline = (int)data.find("\n");
    string line = data.substr(0, endline);

    // NOTE: Support older animation files until we upgrade completely...
    if (line.find("VERSION") != string::npos) {
      string version = ValueOf("VERSION", line);
      if (version != "1.0") legacySupport = false;

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

    data = data.substr(endline + 1);
  } while (endline > -1);

  // One more addAnimation to do if file is good
  if (frameAnimationIndex >= 0) {
    animations.insert(std::make_pair(currentState, frameLists.at(frameAnimationIndex)));
  }
}

string Animation::ValueOf(string _key, string _line) {
  int keyIndex = (int)_line.find(_key);
  // assert(keyIndex > -1 && "Key was not found in .animation file.");
  string s = _line.substr(keyIndex + _key.size() + 2);
  return s.substr(0, s.find("\""));
}

void Animation::Update(float elapsed, sf::Sprite* target, double playbackSpeed) {
  progress += elapsed * (float)std::fabs(playbackSpeed);

  animator(progress, *target, animations[currAnimation]);
}

void Animation::SetFrame(int frame, sf::Sprite * target)
{
  animator.SetFrame(frame, *target, animations[currAnimation]);
}

void Animation::SetAnimation(string state) {
   animator.Clear();
   progress = 0.0f;
   currAnimation = state;

   auto pos = animations.find(currAnimation);

   if (pos == animations.end()) {
     //throw std::runtime_error(std::string("No animation found in file for " + currAnimation));
     std::cout << "No animation found in file for " + currAnimation << std::endl;
   }
}

FrameList & Animation::GetFrameList(std::string animation)
{
  return animations[animation];
}

Animation & Animation::operator<<(Animate::On rhs)
{
  animator << rhs;
  return *this;
}

Animation & Animation::operator<<(Animate::Mode rhs)
{
  animator << rhs;
  return *this;
}

void Animation::operator<<(std::function<void()> onFinish)
{
  animator << onFinish;
}
