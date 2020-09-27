#include <type_traits>
#include <vector>
#include <map>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Activity.h>

#include "bnBattleSceneState.h"
#include "States/bnFadeOutBattleState.h"

class Player; // forward declare

// BattleScene interface provides an API for creating complex states
class IBattleScene : public swoosh::Activity {
    private:
    bool quitting{false}; //!< Determine if we are leaving the battle scene

protected:
    using ChangeCondition = BattleSceneState::ChangeCondition;

    /*
        \brief StateNode represents a node in a graph of conditional states

        States can flow from one to another that it is linked to.
        We call two linked nodes an Edge.
        To transition from one node to the other, the linked condition must be met (true).
        We can link battle scene states together in the inherited IBattleScene class.
    */
    class StateNode {
        friend class IBattleScene;

        BattleSceneState& state; //!< The battle scene state this node represents
        IBattleScene& owner; //!< The scene this state refers to
    public:
        StateNode(IBattleScene& owner, BattleSceneState& state) 
        : state(state), owner(owner)
        {}
    };

    /*
        \brief This wrapper is just a StateNode with a few baked-in API functions to create easy conditional state transitions 
    */
    template<typename T>
    class StateNodeWrapper : public StateNode {
        T& state;

        public:
        using Class = T;

        StateNodeWrapper(IBattleScene& owner, T& state) 
        : state(state), StateNode(owner, state)
        {}

        /* 
            \brief Return the underlining state object as a pointer
        */
        T* operator->() {
            return &state;
        }

        /*
            \brief Return the underlining state object pointer as a reference
        */
        T& operator*() {
            return state;
        }

        /*
            \brief if input functor is a member function, then create a closure to call the function on a class object 
        */
        template<
            typename MemberFunc,
            typename = typename std::enable_if<std::is_member_function_pointer<MemberFunc>::value>::type
        >
        StateNodeWrapper& ChangeOnEvent(StateNode& next, MemberFunc when) {
            T* statePtr = &state;
            owner.Link(*this, next, 
                [statePtr, when]{
                    return (statePtr->*when)();
                });
            return *this;
        }

        /* 
            \brief if input is a lambda, use the ::Link() API function already provided
        */
        template<
            typename Lambda,
            typename = typename std::enable_if<!std::is_member_function_pointer<Lambda>::value>::type
        >
        StateNodeWrapper& ChangeOnEvent(StateNode& next, const Lambda& when) {
            owner.Link(*this, next, when);
            return *this;
        }
    };

    IBattleScene(swoosh::ActivityController* controller, Player* localPlayer) : swoosh::Activity(*controller) {};
    virtual ~IBattleScene() {};

    /*
        \brief Use class type T as the state and perfect-forward arguments to the class 
        \return StateNodeWrapper<T> structure for easy programming
    */
    template<typename T, typename... Args>
    StateNodeWrapper<T> AddState(Args&&... args) {
        T* ptr = new T(std::forward<decltype(args)>(args)...);
        states.insert(states.begin(), ptr);
        return StateNodeWrapper<T>(*this, *ptr);
        using Class = T;
    }

    /*  
        \brief Set the current state pointer to this state node reference and begin the scene
    */
    void Start(StateNode& start) {
        this->current = &start.state;
        this->current->onStart();
    }

    /*
        \brief Update the scene and current state. If any conditions are satisfied, transition to the linked state
    */
    void onUpdate(double elapsed) {
        if(!current) return;
        current->onUpdate(elapsed);

        auto options = nodeToEdges.equal_range(current);

        for(auto iter = options.first; iter != options.second; iter++) {
            if(iter->second.when()) {
                current->onEnd();
                current = &iter->second.b.get().state;
                current->onStart();
            }
        }
    }

public:
    /*
        \brief Forces the creation a fadeout state onto the state pointer and goes back to the last scene
    */
    void Quit(const FadeOut& mode) {
        if(quitting) return; 

        // end the current state
        if(current) {
            current->onEnd();
            delete current;
            current = nullptr;
        }

        // Delete all state transitions
        nodeToEdges.clear();

        for(auto&& state : states) {
            delete state;
        }

        states.clear();

        // Depending on the mode, use Swoosh's 
        // activity controller to fadeout with the right
        // visual appearance
        if(mode == FadeOut::white) {
            getController().queuePop<WhiteWashFade>();
        } else {
            getController().queuePop<BlackWashFade>();
        }

        quitting = true;
    }

private:
    Player* player {nullptr}; //!< Every scene has a pointer to the player

    int round{0}; //!< Some scene types repeat battles and need to track rounds

    BattleSceneState* current{nullptr}; //!< Pointer to the current battle scene state
    std::vector<BattleSceneState*> states; //!< List of all battle scene states

    /*
        \brief Edge represents a link from from state A to state B when a condition is met
    */
    struct Edge {
        std::reference_wrapper<StateNode> a; 
        std::reference_wrapper<StateNode> b; 
        ChangeCondition when; //!< functor that returns boolean
    };

    std::multimap<BattleSceneState*, Edge> nodeToEdges; //!< All edges. Together, they form a graph

    /*
        \brief Creates an edge with a condition and packs it away into the scene graph
    */
    void Link(StateNode& a, StateNode& b, ChangeCondition&& when) {
        Edge edge{a, b, std::move(when)};
        nodeToEdges.insert(std::make_pair(&a.state, edge));
        nodeToEdges.insert(std::make_pair(&b.state, edge));
    }
};
