#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnElecpulse.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ElecPulseCardAction : public CardAction {
private:
    sf::Sprite overlay;
    SpriteProxyNode* attachment;
    Animation attachmentAnim;
    Spell* elecpulse;
    int damage;
public:
    ElecPulseCardAction(Character* owner, int damage);
    ~ElecPulseCardAction();
    void OnUpdate(float _elapsed);
    void EndAction();
    void Execute();
};
