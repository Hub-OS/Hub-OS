#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnElecpulse.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ElecPulseCardAction : public CardAction {
private:
    SpriteProxyNode* attachment;
    Animation attachmentAnim;
    std::shared_ptr<Spell> elecpulse;
    int damage;
public:
    ElecPulseCardAction(std::shared_ptr<Character> actor, int damage);
    ~ElecPulseCardAction();

    void OnAnimationEnd() override;
    void OnActionEnd();
    void OnExecute(std::shared_ptr<Character> user);
};
