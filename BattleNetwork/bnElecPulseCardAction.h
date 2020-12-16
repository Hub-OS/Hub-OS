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
    Spell* elecpulse;
    int damage;
public:
    ElecPulseCardAction(Character& owner, int damage);
    ~ElecPulseCardAction();

    void OnUpdate(double _elapsed);
    void OnAnimationEnd() override;
    void OnEndAction();
    void OnExecute();
};
