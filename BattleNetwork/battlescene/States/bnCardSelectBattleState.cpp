#include "bnCardSelectBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnCard.h"
#include "../../bnPlayer.h"
#include "../../bnSelectedCardsUI.h"
#include "../../bnInputManager.h"

// modals like card cust and battle reward slide in 12px per frame for 10 frames. 60 frames = 1 sec
// modal slide moves 120px in 1/6th of a second
// Per 1 second that is 6*120px in 6*1/6 of a sec = 720px in 1 sec
#define MODAL_SLIDE_PX_PER_SEC 720.0f

CardSelectBattleState::CardSelectBattleState(std::vector<Player*> tracked) : tracked(tracked) {
  // Selection input delays
  heldCardSelectInputCooldown = 0.35f; // 21 frames @ 60fps = 0.35 second
  maxCardSelectInputCooldown = 1 / 12.f; // 5 frames @ 60fps = 1/12th second
  cardSelectInputCooldown = maxCardSelectInputCooldown;

  // Load assets
  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  mobBackdropSprite = sf::Sprite(*LOAD_TEXTURE(MOB_NAME_BACKDROP));
  mobEdgeSprite = sf::Sprite(*LOAD_TEXTURE(MOB_NAME_EDGE));

  mobBackdropSprite.setScale(2.f, 2.f);
  mobEdgeSprite.setScale(2.f, 2.f);
}

Battle::Card** CardSelectBattleState::GetCardPtrList()
{
  return cards;
}

int& CardSelectBattleState::GetCardListLengthAddr()
{
  return cardCount;
}

void CardSelectBattleState::onStart()
{
  CardSelectionCust& cardCust = GetScene().GetCardSelectWidget();

  for (Player* player : tracked) {
    player->SetCharging(false);
    SelectedCardsUI* ui = player->GetFirstComponent<SelectedCardsUI>();

    if (ui) {
      // Clear any card UI queues. they will contain null data.
      ui->LoadCards(0, 0);
    }
  }

  AUDIO.Play(AudioType::CUSTOM_SCREEN_OPEN);

  // slide up the screen a hair
  //camera.MoveCamera(sf::Vector2f(240.f, 140.f), sf::seconds(0.5f));

  // Load the next cards
  cardCust.ResetState();
  cardCust.GetNextCards();

  // Reset state flags
  currState = state::slidein;
  formSelected = false;
}

void CardSelectBattleState::onUpdate(double elapsed)
{
  CardSelectionCust& cardCust = GetScene().GetCardSelectWidget();

  if (!cardCust.IsInView() && currState == state::slidein) {
    cardCust.Move(sf::Vector2f(MODAL_SLIDE_PX_PER_SEC * (float)elapsed, 0));
    return;
  }

  if (!cardCust.IsOutOfView() && currState == state::slideout) {
    cardCust.Move(sf::Vector2f(-MODAL_SLIDE_PX_PER_SEC * (float)elapsed, 0));
    return;
  }

  if (cardCust.IsInView()) {
    currState = state::select;

    if (INPUTx.Has(EventTypes::PRESSED_SCAN_RIGHT) || INPUTx.Has(EventTypes::PRESSED_SCAN_LEFT)) {
      if (cardCust.IsVisible()) {
        cardCust.Hide();
      }
      else {
        cardCust.Reveal();
      }
    }
  }

  if (cardCust.CanInteract() && currState == state::select) {
    if (cardCust.IsCardDescriptionTextBoxOpen()) {
      if (!INPUTx.Has(EventTypes::HELD_QUICK_OPT)) {
        cardCust.CloseCardDescription() ? AUDIO.Play(AudioType::CHIP_DESC_CLOSE, AudioPriority::lowest) : 1;
      }
      else if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {

        cardCust.CardDescriptionConfirmQuestion() ? AUDIO.Play(AudioType::CHIP_CHOOSE) : 1;
        cardCust.ContinueCardDescription();
      }

      cardCust.FastForwardCardDescription(4.0);

      if (INPUTx.Has(EventTypes::PRESSED_UI_LEFT)) {
        cardCust.CardDescriptionYes(); //? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
      else if (INPUTx.Has(EventTypes::PRESSED_UI_RIGHT)) {
        cardCust.CardDescriptionNo(); //? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
    }
    else {
      // there's a wait time between moving ones and moving repeatedly (Sticky keys)
      bool moveCursor = cardSelectInputCooldown <= 0 || cardSelectInputCooldown == heldCardSelectInputCooldown;

      if (INPUTx.Has(EventTypes::PRESSED_UI_LEFT) || INPUTx.Has(EventTypes::HELD_UI_LEFT)) {
        cardSelectInputCooldown -= elapsed;

        if (moveCursor) {
          cardCust.CursorLeft() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

          if (cardSelectInputCooldown <= 0) {
            cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
          }
        }
      }
      else if (INPUTx.Has(EventTypes::PRESSED_UI_RIGHT) || INPUTx.Has(EventTypes::HELD_UI_RIGHT)) {
        cardSelectInputCooldown -= elapsed;

        if (moveCursor) {
          cardCust.CursorRight() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

          if (cardSelectInputCooldown <= 0) {
            cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
          }
        }
      }
      else if (INPUTx.Has(EventTypes::PRESSED_UI_UP) || INPUTx.Has(EventTypes::HELD_UI_UP)) {
        cardSelectInputCooldown -= elapsed;

        if (moveCursor) {
          cardCust.CursorUp() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

          if (cardSelectInputCooldown <= 0) {
            cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
          }
        }
      }
      else if (INPUTx.Has(EventTypes::PRESSED_UI_DOWN) || INPUTx.Has(EventTypes::HELD_UI_DOWN)) {
        cardSelectInputCooldown -= elapsed;

        if (moveCursor) {
          cardCust.CursorDown() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

          if (cardSelectInputCooldown <= 0) {
            cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
          }
        }
      }
      else {
        cardSelectInputCooldown = heldCardSelectInputCooldown;
      }

      if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
        bool performed = cardCust.CursorAction();

        if (cardCust.AreCardsReady()) {
          AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::high);

          cards = cardCust.GetCards();
          cardCount = cardCust.GetCardCount();
          currState = state::slideout;

          Player* player = tracked[0];
          SelectedCardsUI* ui = player->GetFirstComponent<SelectedCardsUI>();

          if (ui) {
            ui->LoadCards(cards, cardCount);
          }

          //camera.MoveCamera(sf::Vector2f(240.f, 160.f), sf::seconds(0.5f));
        }
        else if (performed) {
          if (!cardCust.SelectedNewForm()) {
            AUDIO.Play(AudioType::CHIP_CHOOSE, AudioPriority::highest);
            formSelected = false;
          }
          else {
            formSelected = true;
          }
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::lowest);
        }
      }
      else if (INPUTx.Has(EventTypes::PRESSED_CANCEL) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        cardCust.CursorCancel() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::highest) : 1;
      }
      else if (INPUTx.Has(EventTypes::HELD_QUICK_OPT)) {
        cardCust.OpenCardDescription() ? AUDIO.Play(AudioType::CHIP_DESC, AudioPriority::lowest) : 1;
      }
      else if (INPUTx.Has(EventTypes::HELD_PAUSE)) {
        cardCust.CursorSelectOK() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::lowest) : 1;;
      }
    }
  }

  // Play the WHRR WHRR sound when dark cards are highlighted
  if (cardCust.IsDarkCardSelected()) {
    if (streamVolume == -1) {
      streamVolume = AUDIO.GetStreamVolume();
      AUDIO.SetStreamVolume(streamVolume / 2.f);
    }

    AUDIO.Play(AudioType::DARK_CARD, AudioPriority::high);
  }
  else {
    if (streamVolume != -1) {
      AUDIO.SetStreamVolume(streamVolume);
      streamVolume = -1.f;
    }
  }
}

void CardSelectBattleState::onDraw(sf::RenderTexture& surface)
{
  float nextLabelHeight = 0;
  auto mobList = GetScene().MobList();

  for (int i = 0; i < mobList.size(); i++) {
    const Character& mob = mobList[i].get();
    if (mob.IsDeleted())
      continue;

    std::string name = mob.GetName();
    sf::Text mobLabel = sf::Text(sf::String(name), *font);

    mobLabel.setOrigin(mobLabel.getLocalBounds().width, 0);
    mobLabel.setPosition(470.0f, nextLabelHeight);
    mobLabel.setScale(1.0f, 1.0f);
    mobLabel.setOutlineColor(sf::Color(48, 56, 80));
    mobLabel.setOutlineThickness(2.f);

    float labelWidth = mobLabel.getGlobalBounds().width;
    float labelHeight = mobLabel.getGlobalBounds().height / 2.f;

    mobEdgeSprite.setPosition(470.0f - (labelWidth + 10), -5.f + nextLabelHeight + labelHeight);
    auto edgePos = mobEdgeSprite.getPosition();

    mobBackdropSprite.setPosition(edgePos.x + mobEdgeSprite.getGlobalBounds().width, edgePos.y);

    float scalex = GetScene().getController().getInitialWindowSize().x - mobBackdropSprite.getPosition().x;
    mobBackdropSprite.setScale(scalex, 2.f);

    ENGINE.Draw(mobEdgeSprite, false);
    ENGINE.Draw(mobBackdropSprite, false);
    ENGINE.Draw(mobLabel, false);

    // make the next label relative to this one
    nextLabelHeight += mobLabel.getLocalBounds().height + 10.f;
  }

  ENGINE.Draw(GetScene().GetCardSelectWidget());
}

void CardSelectBattleState::onEnd()
{
}

bool CardSelectBattleState::OKIsPressed() {
  return GetScene().GetCardSelectWidget().IsOutOfView();
}

bool CardSelectBattleState::HasForm()
{
  return OKIsPressed() && formSelected;
}
