#pragma once

#include <functional>
#include <type_traits>
#include <SFML/Graphics/RenderTexture.hpp>
#include <Swoosh/ActivityController.h>

class BattleSceneBase;

using namespace swoosh;

/*
    Interface for Battle Scene States
*/
struct BattleSceneState {
    friend class BattleSceneBase;

    BattleSceneBase& GetScene() { return *scene; }
    const BattleSceneBase& GetScene() const { return *scene; }

    ActivityController& GetController() { return *controller; }
    
    virtual void onStart(const BattleSceneState* next=nullptr) = 0;
    virtual void onEnd(const BattleSceneState* last=nullptr) = 0;
    virtual void onUpdate(double elapsed) = 0;
    virtual void onDraw(sf::RenderTexture& surface) = 0;
    virtual ~BattleSceneState(){};

    using ChangeCondition = std::function<bool()>;
private:
  BattleSceneBase* scene{ nullptr };
  ActivityController* controller{ nullptr };
};

// Handy macro to avoid C++ ugliness
#define CHANGE_ON_EVENT(from, to, EventFunc) from.ChangeOnEvent(to, &decltype(from)::Class::EventFunc) 