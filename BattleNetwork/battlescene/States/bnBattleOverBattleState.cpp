#include "bnBattleOverBattleState.h"

#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

bool BattleOverBattleState::IsFinished() {
    return true;
}

BattleOverBattleState::BattleOverBattleState()
{
  battleEnd = sf::Sprite(*LOAD_TEXTURE(ENEMY_DELETED));
  battleEnd.setOrigin(battleEnd.getLocalBounds().width / 2.0f, battleEnd.getLocalBounds().height / 2.0f);
  battleEnd.setPosition(sf::Vector2f(240.f, 140.f));
  battleEnd.setScale(2.f, 2.f);
}

void BattleOverBattleState::onStart()
{
  AUDIO.Stream("resources/loops/enemy_deleted.ogg");
}

void BattleOverBattleState::onUpdate(double elapsed)
{
}
