#pragma once

#include <functional>
#include <type_traits>
#include <SFML/Graphics/RenderTexture.hpp>

class BattleSceneBase;

/*
    Interface for Battle Scene States
*/
struct BattleSceneState {
    friend class BattleSceneBase;

    BattleSceneBase& GetScene() { return *scene; }
    virtual void onStart() = 0;
    virtual void onEnd() = 0;
    virtual void onUpdate(double elapsed) = 0;
    virtual void onDraw(sf::RenderTexture& surface) = 0;
    virtual ~BattleSceneState(){};

    using ChangeCondition = std::function<bool()>;
private:
  BattleSceneBase* scene{ nullptr };
};

// Handy macro to avoid C++ ugliness
#define CHANGE_ON_EVENT(from, to, EventFunc) from.ChangeOnEvent(to, &decltype(from)::Class::EventFunc) 