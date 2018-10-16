#pragma once
#include "bnScene.h"
#include "bnTimer.h"
#include <SFML/Graphics.hpp>
#include <stack>

class SceneManager {
private:
  std::stack<Scene*> scenes;
  Scene* outgoing;
  Timer transition;
  sf::Time duration;
  sf::RenderTexture* top;
  sf::RenderTexture* last;

  void FreeLastHandle() {
    if (outgoing) {
      outgoing->OnEnd();
      delete outgoing;
    }

    outgoing = nullptr;
  }

public:
  SceneManager() {
    top = nullptr;
    last = nullptr;
    duration = sf::seconds(1.0f);
    transition.Start();
    outgoing = nullptr;
  }

  ~SceneManager() {
    while (!scenes.empty()) {
      Scene* scene = scenes.top();
      scene->OnEnd();
      delete scene;
      scenes.pop();
    }

    FreeLastHandle();
  }

  template<class T>
  void PushScene(sf::Time _duration) {
    duration = _duration;
    transition.Reset();

    FreeLastHandle();

    outgoing = scenes.top();
    outgoing->OnLeave();

    Scene* next = new T();
    next->OnStart();

    scene.push(next);
  }

  void PopScene() {
    FreeLastHandle();

    outgoing = scenes.top();
    outgoing->OnLeave();

    scenes.pop();

    if (scenes.top()) {
      scenes.top()->OnResume();
    }
  }

  void Update(double _elapsed) {
    scenes.top()->OnUpdate(_elapsed);

    if (outgoing) {
      outgoing->OnUpdate(_elapsed);
    }
  }

  void Draw(sf::RenderWindow& window) {
    scenes.top()->OnDraw(top);

    if (outgoing) {
      outgoing->OnDraw(last);
    }

    // TODO: mix the two based on transition type and progress
    //window.draw(top);
    //window.draw(last);
  }

};