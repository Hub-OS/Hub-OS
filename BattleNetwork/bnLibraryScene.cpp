#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnWebClientMananger.h"
#include "bnLibraryScene.h"
#include "bnCardLibrary.h"
#include "bnCardFolder.h"
#include "Android/bnTouchArea.h"

#include "bnMessage.h"
#include "bnFont.h"

#include <SFML/Graphics.hpp>
#include <cmath>

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;

using namespace swoosh::types;
using swoosh::types::direction;

#include "Segues/PushIn.h"

std::string LibraryScene::FormatCardDesc(const std::string && desc)
{
  std::string output = desc;

  auto bothAreSpaces = [](char lhs, char rhs) -> bool { return (lhs == rhs) && (lhs == ' '); };

  std::string::iterator new_end = std::unique(output.begin(), output.end(), bothAreSpaces);
  output.erase(new_end, output.end());

  int index = 0;
  int perLine = 0;
  int line = 0;
  int wordIndex = 0; // If we are breaking on a word

  bool finished = false;

  while (index != output.size() && !finished) {
    if (output[index] != ' ' && wordIndex == -1) {
      wordIndex = index;
    }
    else if (output[index] == ' ' && wordIndex > -1) {
      wordIndex = -1;
    }

    if (perLine > 0 && perLine % 9 == 0) {
      if (wordIndex > -1) {
        if (index == wordIndex + 9) {
          wordIndex = index; // We have no choice but to cut off part of this lengthy word
        }
        // Line break at the last word
        //while (desc[index] == ' ') { index++; }
        output.insert(wordIndex, "\n");
        index = wordIndex; // Start counting from here
        while (output[index] == ' ') { output.erase(index); }
      }
      else {
        // Line break at the next word
        while (output[index] == ' ') { index++; }
        output.insert(index, "\n"); index++;
        while (output[index] == ' ') { output.erase(index); }
      }
      line++;

      if (line == 3) {
        output = output.substr(0, index+1);
        output[output.size() - 1] = ';'; // font glyph will show an elipses
        finished = true;
      }

      perLine = 0;
      wordIndex = -1;
    }

    perLine++;
    index++;
  }

  // Card docks can only fit 9 characters per line, 3 lines total
  if (output.size() > 3 * 10) {
    output = output.substr(0, (3 * 10));
    output[output.size() - 1] = ';'; // font glyph will show an elipses
  }

  return output;
}

LibraryScene::LibraryScene(swoosh::ActivityController &controller) :
  camera(getView()),
  textbox(sf::Vector2f(4, 255)),
  font(Font::Style::thick),
  menuLabel("", font),
  cardFont(Font::Style::wide),
  cardLabel("", cardFont),
  cardDescFont(Font::Style::small),
  cardDesc("", cardDescFont),
  numberFont(Font::Style::wide),
  numberLabel("", numberFont),
  Scene(controller)
{

  // Menu name font
  menuLabel.setPosition(sf::Vector2f(20.f, 5.0f));

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second

#ifdef __ANDROID__
  maxSelectInputCooldown = 0.1;
  canSwipe = false;
  releasedB = false;
  touchStart = false;
  touchPosX = touchPosStartX = -1;
#endif

  selectInputCooldown = maxSelectInputCooldown;

  // Card UI font
  cardLabel.setPosition(275.f, 15.f);

  numberLabel.SetColor(sf::Color(48, 56, 80));
  numberLabel.setScale(0.8f, 0.8f);
  numberLabel.setOrigin(numberLabel.GetLocalBounds().width, 0);
  numberLabel.setPosition(sf::Vector2f(170.f, 28.0f));

  // Card description font
  cardDesc.setPosition(sf::Vector2f(20.f, 185.0f));
  //cardDesc.setLineSpacing(5);
  cardDesc.SetColor(sf::Color::Black);

  // folder menu graphic
  bg.setTexture(LOAD_TEXTURE(FOLDER_VIEW_BG));
  bg.setScale(2.f, 2.f);

  folderDock.setTexture(LOAD_TEXTURE(FOLDER_DOCK));
  folderDock.setScale(2.f, 2.f);
  folderDock.setPosition(2.f, 30.f);

  scrollbar.setTexture(LOAD_TEXTURE(FOLDER_SCROLLBAR));
  scrollbar.setScale(2.f, 2.f);
  scrollbar.setPosition(410.f, 60.f);

  cursor.setTexture(LOAD_TEXTURE(FOLDER_CURSOR));
  cursor.setScale(2.f, 2.f);
  cursor.setPosition((2.f*90.f), 64.0f);

  stars.setTexture(LOAD_TEXTURE(FOLDER_RARITY));
  stars.setScale(2.f, 2.f);

  cardHolder.setTexture(LOAD_TEXTURE(FOLDER_CHIP_HOLDER));
  cardHolder.setScale(2.f, 2.f);
  cardHolder.setPosition(4.f, 35.f);

  element.setTexture(LOAD_TEXTURE(ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(2.f*25.f, 146.f);

  // Current card graphic
  card = sf::Sprite();
  cardSubFrame = sf::IntRect(0,0,56,64);
  card.setTextureRect(cardSubFrame);
  card.setScale(2.f, 2.f);
  card.setPosition(83.f, 93.f);
  card.setOrigin(card.getLocalBounds().width / 2.0f, card.getLocalBounds().height / 2.0f);

  auto iconRect = sf::IntRect(0, 0, 14, 14);
  cardIcon.setTextureRect(iconRect);
  cardIcon.setScale(2.f, 2.f);

  cardRevealTimer.start();
  easeInTimer.start();

  maxCardsOnScreen = 7;
  currCardIndex = lastCardOnScreen = prevIndex = 0;
  totalTimeElapsed = frameElapsed = 0.0;

  MakeUniqueCardsFromPack();
}

LibraryScene::~LibraryScene() { ; }

void LibraryScene::MakeUniqueCardsFromPack()
{
  CardLibrary::Iter iter = CHIPLIB.Begin();

  for (iter; iter != CHIPLIB.End(); iter++) {
    uniqueCards.insert(uniqueCards.begin(), *iter);
  }

  auto pred = [](const Battle::Card &a, const Battle::Card &b) -> bool {
    return a.GetShortName() == b.GetShortName();
  };

  uniqueCards.unique(pred);

  numOfCards = (int)uniqueCards.size();
}

void LibraryScene::onStart() {
  gotoNextScene = false;

#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void LibraryScene::onUpdate(double elapsed) {
#ifdef __ANDROID__
  if(gotoNextScene)
    return; // keep the screen looking the same when we come into
#endif

  frameElapsed = elapsed;
  totalTimeElapsed += elapsed;

  cardRevealTimer.update(sf::seconds(static_cast<float>(elapsed)));
  easeInTimer.update(sf::seconds(static_cast<float>(elapsed)));
  camera.Update(elapsed);
  textbox.Update(elapsed);

  // Scene keyboard controls
  if (!gotoNextScene) {
    if (Input().Has(InputEvents::pressed_ui_up)) {
      selectInputCooldown -= elapsed;

      prevIndex = currCardIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currCardIndex--;
        Audio().Play(AudioType::CHIP_SELECT);

        if (currCardIndex < lastCardOnScreen) {
          --lastCardOnScreen;
        }

        cardRevealTimer.reset();
      }
    }
    else if (Input().Has(InputEvents::pressed_ui_down)) {
      selectInputCooldown -= elapsed;

      prevIndex = currCardIndex;

      if (selectInputCooldown <= 0) {
        selectInputCooldown = maxSelectInputCooldown;
        currCardIndex++;
        Audio().Play(AudioType::CHIP_SELECT);

        if (currCardIndex > lastCardOnScreen + maxCardsOnScreen - 1) {
          ++lastCardOnScreen;
        }

        cardRevealTimer.reset();
      }
    }
    else {
      selectInputCooldown = 0;
    }

    if (Input().Has(InputEvents::pressed_confirm) && textbox.IsClosed()) {
      auto iter = uniqueCards.begin();
      int i = 0;

      while (i < currCardIndex) {
        i++;
        iter++;
      }

      textbox.DequeMessage(); // make sure textbox is empty
      textbox.EnqueMessage(sf::Sprite(), "", new Message(iter->GetVerboseDescription()));
      textbox.Open();
      Audio().Play(AudioType::CHIP_DESC);
    }
    else if (Input().Has(InputEvents::released_cancel) && textbox.IsOpen()) {
      textbox.Close();
      textbox.SetTextSpeed(1.0);
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
    }
    else if (Input().Has(InputEvents::pressed_confirm) && textbox.IsOpen()) {
      textbox.SetTextSpeed(3.0);
    }
    else if (Input().Has(InputEvents::released_confirm) && textbox.IsOpen()) {
      textbox.SetTextSpeed(1.0);
      //textbox.Continue();
    }

    currCardIndex = std::max(0, currCardIndex);
    currCardIndex = std::min(numOfCards - 1, currCardIndex);

    lastCardOnScreen = std::max(0, lastCardOnScreen);
    lastCardOnScreen = std::min(numOfCards - 1, lastCardOnScreen);

    if (Input().Has(InputEvents::pressed_cancel) && textbox.IsClosed()) {
      gotoNextScene = true;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);

      using segue = segue<PushIn<direction::left>, milli<500>>;
      getController().pop<segue>();
    }
  }

}

void LibraryScene::onLeave() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}

void LibraryScene::onExit()
{
}

void LibraryScene::onEnter()
{
}

void LibraryScene::onResume() {
#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void LibraryScene::onDraw(sf::RenderTexture& surface) {
  surface.draw(bg);
  surface.draw(menuLabel);

  surface.draw(folderDock);
  surface.draw(cardHolder);

  // ScrollBar limits: Top to bottom screen position when selecting first and last card respectively
  float top = 50.0f; float bottom = 230.0f;
  float depth = ((float)lastCardOnScreen / (float)numOfCards)*bottom;
  scrollbar.setPosition(452.f, top + depth);

  surface.draw(scrollbar);

  if (uniqueCards.size() == 0) return;

  // Move the card library iterator to the current highlighted card
  auto iter = uniqueCards.begin();

  for (int j = 0; j < lastCardOnScreen; j++) {
    iter++;
  }

  // Now that we are at the viewing range, draw each card in the list
  for (int i = 0; i < maxCardsOnScreen && lastCardOnScreen + i < numOfCards; i++) {
    cardIcon.setTexture(WEBCLIENT.GetIconForCard(iter->GetUUID()));
    cardIcon.setPosition(2.f*104.f, 65.0f + (32.f*i));
    surface.draw(cardIcon);

    cardLabel.setOrigin(0, 0);
    cardLabel.SetColor(sf::Color::White);
    cardLabel.setPosition(2.f*120.f, 60.0f + (32.f*i));
    cardLabel.SetString(iter->GetShortName());
    surface.draw(cardLabel);

    // Draw cursor
    if (lastCardOnScreen + i == currCardIndex) {
      auto y = swoosh::ease::interpolate((float)frameElapsed*7.f, cursor.getPosition().y, 64.0f + (32.f*i));
      auto bounce = std::sin((float)totalTimeElapsed*10.0f)*5.0f;

      cursor.setPosition((2.f*90.f) + bounce, y);
      surface.draw(cursor);

      card.setScale((float)swoosh::ease::linear(cardRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f)*2.0f, 2.0f);
      surface.draw(card);

      // This draws the currently highlighted card
      if (iter->GetDamage() > 0) {
        cardLabel.SetColor(sf::Color::White);
        cardLabel.SetString(std::to_string(iter->GetDamage()));
        cardLabel.setOrigin(cardLabel.GetLocalBounds().width + cardLabel.GetLocalBounds().left, 0);
        cardLabel.setPosition(2.f*(70.f), 135.f);

        surface.draw(cardLabel);
      }

      std::string formatted = FormatCardDesc(iter->GetDescription());
      cardDesc.SetString(formatted);
      surface.draw(cardDesc);

      int offset = (int)(iter->GetElement());
      auto elementRect = sf::IntRect(14 * offset, 0, 14, 14);
      element.setTextureRect(elementRect);
      element.setPosition(2.f*25.f, 142.f);
      surface.draw(element);
    }

    iter++;
  }

  surface.draw(textbox);
}

void LibraryScene::onEnd() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}


#ifdef __ANDROID__
void LibraryScene::StartupTouchControls() {
  /* Android touch areas*/
  TouchArea& rightSide = TouchArea::create(sf::IntRect(240, 0, 240, 320));

  rightSide.enableExtendedRelease(true);

  rightSide.onTouch([]() {
      INPUTx.VirtualKeyEvent(InputEvent::RELEASED_A);
  });

  rightSide.onRelease([this](sf::Vector2i delta) {
      if(!releasedB) {
        INPUTx.VirtualKeyEvent(InputEvent::PRESSED_A);
      }
  });

  rightSide.onDrag([this](sf::Vector2i delta){
      if(delta.x < -25 && !releasedB) {
        INPUTx.VirtualKeyEvent(InputEvent::PRESSED_B);
        INPUTx.VirtualKeyEvent(InputEvent::RELEASED_B);
        releasedB = true;
      }
  });
}

void LibraryScene::ShutdownTouchControls() {
  TouchArea::free();
}
#endif
