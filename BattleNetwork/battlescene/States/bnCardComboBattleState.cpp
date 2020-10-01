#include "bnCardComboBattleState.h"

#include "../battlescene/bnBattleSceneBaseBase.h"
#include "../../bnTextureResourceManager.h"
#include "../../bnAudioResourceManager.h"

#include <SFML/Graphics/Font.hpp>

CardComboBattleState::CardComboBattleState(SelectedCardsUI& ui, PA& programAdvance) : ui(ui), font(), programAdvance(programAdvance) {
  /*
  Program Advance + labels
  */
  hasPA = -1;
  paStepIndex = 0;

  listStepCooldown = 0.2f;
  listStepCounter = listStepCooldown;

  programAdvanceSprite = sf::Sprite(*LOAD_TEXTURE(PROGRAM_ADVANCE));
  programAdvanceSprite.setScale(2.f, 2.f);
  programAdvanceSprite.setOrigin(0, programAdvanceSprite.getLocalBounds().height / 2.0f);
  programAdvanceSprite.setPosition(40.0f, 58.f);
}

void CardComboBattleState::ShareCardList(Battle::Card** cards, int* listLengthPtr)
{
  this->cards = cards;
  this->cardCountPtr = listLengthPtr;
}

void CardComboBattleState::onStart()
{
  isPAComplete = false;
}

void CardComboBattleState::onEnd()
{
  ENGINE.RevokeShader();
}

void CardComboBattleState::onUpdate(double elapsed)
{
  CardSelectionCust& cardCust = GetScene().GetCardSelectWidget();

  // we're leaving a state
  // Start Program Advance checks
  if (isPAComplete && hasPA == -1) {
    // Filter and apply support cards
    GetScene().FilterSupportCards(cards, *cardCountPtr);

    // Return to game
    ui.LoadCards(cards, *cardCountPtr);
  }
  else if (!isPAComplete) {

    hasPA = programAdvance.FindPA(cards, *cardCountPtr);

    if (hasPA > -1) {
      paSteps = programAdvance.GetMatchingSteps();
      PAStartTimer.reset();
    }

    isPAComplete = true;
  }
  else if (hasPA > -1) {
    static bool advanceSoundPlay = false;
    static float increment = 0;

    float nextLabelHeight = 0;

    double PAStartSecs = PAStartTimer.getElapsed().asSeconds();
    double scale = swoosh::ease::linear(PAStartSecs, PAStartLength, 1.0);
    programAdvanceSprite.setScale(2.f, (float)scale * 2.f);
    ENGINE.Draw(programAdvanceSprite, false);

    if (paStepIndex <= (*cardCountPtr) + 1) {
      for (int i = 0; i < paStepIndex && i < *cardCountPtr; i++) {
        std::string formatted = cards[i]->GetShortName();
        formatted.resize(9, ' ');
        formatted[8] = cards[i]->GetCode();

        sf::Text stepLabel = sf::Text(formatted, font);

        stepLabel.setOrigin(0, 0);
        stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight * 2.f));
        stepLabel.setScale(1.0f, 1.0f);

        if (i >= hasPA && i <= hasPA + paSteps.size() - 1) {
          if (i < paStepIndex - 1) {
            stepLabel.setOutlineColor(sf::Color(0, 0, 0));
            stepLabel.setFillColor(sf::Color(128, 248, 80));
          }
          else {
            stepLabel.setOutlineColor(sf::Color(0, 0, 0));
            stepLabel.setFillColor(sf::Color(247, 188, 27));
          }
        }
        else {
          stepLabel.setOutlineColor(sf::Color(48, 56, 80));
        }

        stepLabel.setOutlineThickness(2.f);
        ENGINE.Draw(stepLabel, false);

        // make the next label relative to this one
        nextLabelHeight += stepLabel.getLocalBounds().height;
      }
      increment = 0;
      nextLabelHeight = 0;
    }
    else {
      if (!advanceSoundPlay) {
        AUDIO.Play(AudioType::PA_ADVANCE);
        advanceSoundPlay = true;
      }

      for (int i = 0; i < *cardCountPtr; i++) {
        std::string formatted = cards[i]->GetShortName();
        formatted.resize(9, ' ');
        formatted[8] = cards[i]->GetCode();

        sf::Text stepLabel = sf::Text(formatted, font);

        stepLabel.setOrigin(0, 0);
        stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight * 2.f));
        stepLabel.setScale(1.0f, 1.0f);
        stepLabel.setOutlineColor(sf::Color(48, 56, 80));
        stepLabel.setOutlineThickness(2.f);

        if (i >= hasPA && i <= hasPA + paSteps.size() - 1) {
          if (i == hasPA) {
            Battle::Card* paCard = programAdvance.GetAdvanceCard();

            sf::Text stepLabel = sf::Text(paCard->GetShortName(), font);
            stepLabel.setOrigin(0, 0);
            stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight * 2.f));
            stepLabel.setScale(1.0f, 1.0f);

            stepLabel.setOutlineColor(sf::Color((sf::Uint32)(sin(increment) * 255), (sf::Uint32)(cos(increment + 90 * (22.f / 7.f)) * 255), (sf::Uint32)(sin(increment + 180 * (22.f / 7.f)) * 255)));
            stepLabel.setOutlineThickness(2.f);
            ENGINE.Draw(stepLabel, false);
          }
          else {
            // make the next label relative to the hidden one and skip drawing
            nextLabelHeight += stepLabel.getLocalBounds().height;

            continue;
          }

        }
        else {
          ENGINE.Draw(stepLabel, false);
        }

        // make the next label relative to this one
        nextLabelHeight += stepLabel.getLocalBounds().height;
      }

      increment += (float)elapsed * 5.f;
    }

    if (listStepCounter > 0.f) {
      listStepCounter -= (float)elapsed;
    }
    else {
      // +2 = 1 step for showing PA label and 1 step for showing merged card
      // That's the cards we want to show + 1 + 1 = cardCount + 2
      if (paStepIndex == (*cardCountPtr) + 2) {
        advanceSoundPlay = false;

        Battle::Card* paCard = programAdvance.GetAdvanceCard();

        // Only remove the cards involved in the program advance. Replace them with the new PA card.
        // PA card is dealloc by the class that created it so it must be removed before the library tries to dealloc
        int newCardCount = (*cardCountPtr) - (int)paSteps.size() + 1; // Add the new one
        int newCardStart = hasPA;

        // Create a temp card list
        Battle::Card** newCardList = new Battle::Card * [newCardCount];

        int j = 0;
        for (int i = 0; i < *cardCountPtr; ) {
          if (i == hasPA) {
            newCardList[j] = paCard;
            i += (int)paSteps.size();
            j++;
            continue;
          }

          newCardList[j] = cards[i];
          i++;
          j++;
        }

        // Set the new cards
        for (int i = 0; i < newCardCount; i++) {
          cards[i] = *(newCardList + i);
        }

        // Delete the temp list space
        // NOTE: We are _not_ deleting the pointers in them
        delete[] newCardList;

        *cardCountPtr = newCardCount;
      }
      else {
        if (paStepIndex == (*cardCountPtr) + 1) {
          listStepCounter = listStepCooldown * 2.0f; // Linger on the screen when showing the final PA
        }
        else {
          listStepCounter = listStepCooldown * 0.7f; // Quicker about non-PA cards
        }

        if (paStepIndex >= hasPA && paStepIndex <= hasPA + paSteps.size() - 1) {
          listStepCounter = listStepCooldown; // Take our time with the PA cards
          AUDIO.Play(AudioType::POINT_SFX);
        }

        paStepIndex++;
      }
    }
  }
}

bool CardComboBattleState::IsDone() {
    return isPAComplete;
}