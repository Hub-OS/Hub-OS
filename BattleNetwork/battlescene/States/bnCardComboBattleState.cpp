#include "bnCardComboBattleState.h"

#include "../../bnTextureResourceManager.h"

CardComboBattleState::CardComboBattleState() {
  /*
  Program Advance + labels
  */
  isPAComplete = false;
  hasPA = -1;
  paStepIndex = 0;

  listStepCooldown = 0.2f;
  listStepCounter = listStepCooldown;

  programAdvanceSprite = sf::Sprite(*LOAD_TEXTURE(PROGRAM_ADVANCE));
  programAdvanceSprite.setScale(2.f, 2.f);
  programAdvanceSprite.setOrigin(0, programAdvanceSprite.getLocalBounds().height / 2.0f);
  programAdvanceSprite.setPosition(40.0f, 58.f);
}

bool CardComboBattleState::IsDone() {
    return isPAComplete;
}