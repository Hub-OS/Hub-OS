#include <functional>
#include <type_traits>

/*
    Interface for Battle Scene States
*/
struct BattleSceneState {
    virtual void onStart() = 0;
    virtual void onEnd() = 0;
    virtual void onUpdate(double elapsed) = 0;
    virtual void onDraw(double elapsed) = 0;
    virtual ~BattleSceneState(){};

    using ChangeCondition = std::function<bool()>;
};

// Handy macro to avoid C++ ugliness
#define CHANGE_ON_EVENT(from, to, EventFunc) from.ChangeOnEvent(to, &decltype(from)::Class::EventFunc) 