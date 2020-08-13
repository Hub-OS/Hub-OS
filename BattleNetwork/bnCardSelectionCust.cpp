#include "bnCardSelectionCust.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnInputManager.h"
#include "bnWebClientMananger.h"
#include "bnCardLibrary.h"

#define WILDCARD '*'
#define VOIDED 0
#define STAGED 1
#define QUEUED 2

CardSelectionCust::CardSelectionCust(CardFolder* _folder, int cap, int perTurn) :
    perTurn(perTurn),
    greyscale(*SHADERS.GetShader(ShaderType::GREYSCALE)),
    cardDescriptionTextbox(sf::Vector2f(4, 255)),
    isInView(false),
    isInFormSelect(false),
    playFormSound(false),
    canInteract(true),
    isDarkCardSelected(false),
    darkCardShadowAlpha(0)
{
    frameElapsed = 1;
    folder = _folder;
    cap = std::min(cap, 8);
    cardCap = cap;
    queue = new Bucket[cardCap];
    selectQueue = new Bucket*[cardCap];

    cardCount = selectCount = cursorPos = cursorRow = 0;

    emblem.setScale(2.f, 2.f);
    emblem.setPosition(194.0f, 14.0f);

    custSprite = sf::Sprite(*LOAD_TEXTURE(CHIP_SELECT_MENU));
    custSprite.setScale(2.f, 2.f);
    custSprite.setPosition(-custSprite.getTextureRect().width*2.f, 0);

    custDarkCardOverlay = sf::Sprite(*LOAD_TEXTURE(CHIP_SELECT_DARK_OVERLAY));
    custDarkCardOverlay.setScale(2.f, 2.f);
    custDarkCardOverlay.setPosition(custSprite.getPosition());

    custMegaCardOverlay = custDarkCardOverlay;
    custMegaCardOverlay.setTexture(*LOAD_TEXTURE(CHIP_SELECT_MEGA_OVERLAY));

    custGigaCardOverlay = custDarkCardOverlay;
    custGigaCardOverlay.setTexture(*LOAD_TEXTURE(CHIP_SELECT_GIGA_OVERLAY));

    // TODO: fully use scene nodes on all card slots and the GUI sprite
    // AddSprite(custSprite);

    icon.setTextureRect(sf::IntRect{ 0,0,14,14 });
    icon.setScale(sf::Vector2f(2.f, 2.f));

    element.setTexture(TEXTURES.GetTexture(TextureType::ELEMENT_ICON));
    element.setScale(2.f, 2.f);
    element.setPosition(2.f*25.f, 146.f);

    cursorSmall = sf::Sprite(*TEXTURES.GetTexture(TextureType::CHIP_CURSOR_SMALL));
    cursorSmall.setScale(sf::Vector2f(2.f, 2.f));

    cursorBig = sf::Sprite(*TEXTURES.GetTexture(TextureType::CHIP_CURSOR_BIG));
    cursorBig.setScale(sf::Vector2f(2.f, 2.f));

    // never moves
    cursorBig.setPosition(sf::Vector2f(2.f*104.f, 2.f*122.f));

    cardLock = sf::Sprite(*LOAD_TEXTURE(CHIP_LOCK));
    cardLock.setScale(sf::Vector2f(2.f, 2.f));

    cardCard.setScale(2.f, 2.f);
    cardCard.setPosition(2.f*16.f, 48.f);
    cardCard.setTextureRect(sf::IntRect{0, 0, 56, 48});

    cardNoData.setTexture(TEXTURES.GetTexture(TextureType::CHIP_NODATA));
    cardNoData.setScale(2.f, 2.f);
    cardNoData.setPosition(2.f*16.f, 48.f);

    cardSendData.setTexture(TEXTURES.GetTexture(TextureType::CHIP_SENDDATA));
    cardSendData.setScale(2.f, 2.f);
    cardSendData.setPosition(2.f*16.f, 48.f);

    labelFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
    label.setFont(*labelFont);

    codeFont = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
    smCodeLabel.setFont(*codeFont);
    smCodeLabel.setCharacterSize(12);
    smCodeLabel.setFillColor(sf::Color::Yellow);

    cursorSmallAnimator = Animation("resources/ui/cursor_small.animation");
    cursorSmallAnimator.Reload();
    cursorSmallAnimator.SetAnimation("BLINK");
    cursorSmallAnimator << Animator::Mode::Loop;

    cursorBigAnimator = Animation("resources/ui/cursor_big.animation");
    cursorBigAnimator.Reload();
    cursorBigAnimator.SetAnimation("BLINK");
    cursorBigAnimator << Animator::Mode::Loop;

    formSelectAnimator = Animation("resources/ui/form_select.animation");
    formSelectAnimator.Reload();
    formSelectAnimator.SetAnimation("CLOSED");

    formCursorAnimator = Animation("resources/ui/form_cursor.animation");
    formCursorAnimator.Reload();
    formCursorAnimator.SetAnimation("BLINK");
    formCursorAnimator << Animator::Mode::Loop;

    formSelectQuitTimer = 0.f; // used to time out the activation
    thisFrameSelectedForm = selectedForm = -1;

    formItemBG.setTexture(*LOAD_TEXTURE(CUST_FORM_ITEM_BG));
    formItemBG.setScale(2.f, 2.f);

    formSelect.setTexture(LOAD_TEXTURE(CUST_FORM_SELECT));
    formCursor.setTexture(LOAD_TEXTURE(CUST_FORM_CURSOR));

    formSelect.setScale(2.f, 2.f);
    formCursor.setScale(2.f, 2.f);

    //setScale(0.5f, 0.5); // testing transforms
}


CardSelectionCust::~CardSelectionCust() {
  ClearCards();

  for (int i = 0; i < cardCount; i++) {
    // selectQueue[i] = nullptr;
    // delete queue[i].data;
  }

  if (cardCount > 0) {
    delete[] queue;
    delete[] selectQueue;
  }

  cardCount = 0;

  delete folder;
}

bool CardSelectionCust::CursorUp() {
  if (isInFormSelect) {
    if (--formCursorRow < 0) {
      formCursorRow = 0;
      return false;
    }

    return true;
  }
  else {
    if (--cursorRow < 0) {
      cursorRow = 0;

      if (forms.size()) {
        auto onEnd = [this]() {
          formSelectAnimator << "OPEN";
        };

        formSelectAnimator << "OPENING" << onEnd;
        isInFormSelect = true;
        cursorPos = cursorRow = 0;
        AUDIO.Play(AudioType::CHIP_DESC);
      }
      return false;
    }

    return true;
  }
}

bool CardSelectionCust::CursorDown() {
  if (isInFormSelect) {
    if (forms.size() == 0) return false;

    if (++formCursorRow >= forms.size()) {
      formCursorRow = int(forms.size() - 1);

      return false;
    }

    return true;
  }
  else {
    if (++cursorRow > 1) {
      cursorRow = 1;

      return false;
    }

    if (cursorPos > 2) {
      cursorPos = 0;
    }

    return true;
  }
}

bool CardSelectionCust::CursorRight() {
  if (isInFormSelect) return false;

  if (++cursorPos > 2 && cursorRow == 1) {
    cursorPos = 0;
  }
  else if (cursorPos > 5 && cursorRow == 0) {
    cursorPos = 0;
  }

  return true;
}

bool CardSelectionCust::CursorLeft() {
  if (isInFormSelect) return false;

  if (--cursorPos < 0 && cursorRow == 1) {
    cursorPos = 2;
  }
  else if (cursorPos < 0 && cursorRow == 0) {
    cursorPos = 5;
  }

  return true;
}

bool CardSelectionCust::CursorAction() {
  if (isInFormSelect) {
    auto res = true;

    thisFrameSelectedForm = formCursorRow;

    if (thisFrameSelectedForm == selectedForm) {
      res = true;
      selectedForm = thisFrameSelectedForm = -1; // no change
    }
    else {
      formSelectQuitTimer = 0.5f; // 0.5 * 60fps = 30 frames
      selectedForm = thisFrameSelectedForm;
    }
    return res;
  }

  // Should never happen but just in case
  if (selectCount > 5) {
    return false;
  }

  if (cursorPos == 5 && cursorRow == 0) {
    // End card select
    areCardsReady = true;
  } else {
    // Does card exist 
    if (cursorPos + (5 * cursorRow) < cardCount) {
      // Queue this card if not selected
      if (queue[cursorPos + (5 * cursorRow)].state == STAGED) {
        selectQueue[selectCount++] = &queue[cursorPos + (5 * cursorRow)];
        queue[cursorPos + (5 * cursorRow)].state = QUEUED;

        // We can only upload 5 cards to navi...
        if (selectCount == 5) {
          for (int i = 0; i < cardCount; i++) {
            if (queue[i].state == QUEUED) continue;

            queue[i].state = VOIDED;
          }

          emblem.CreateWireEffect();
          return true;
        }
        else {
          // Otherwise display the remaining compatible cards...
          // Check card code. If other cards are not compatible, set their bucket state flag to 0
          Battle::Card* card = queue[cursorPos + (5 * cursorRow)].data;
          char code = card->GetCode();
          bool isDark = card->GetClass() == Battle::CardClass::dark;

          for (int i = 0; i < cardCount; i++) {
            if (i == cursorPos + (5 * cursorRow) || (queue[i].state == VOIDED) || queue[i].state == QUEUED) continue;
            char otherCode = queue[i].data->GetCode();
            bool isOtherDark = queue[i].data->GetClass() == Battle::CardClass::dark;
            bool isSameCard = (queue[i].data->GetShortName() == queue[cursorPos + (5 * cursorRow)].data->GetShortName());
            bool canPair = (code == WILDCARD || otherCode == WILDCARD || otherCode == code || isSameCard);
            if (isOtherDark == isDark && canPair) { queue[i].state = STAGED; }
            else { queue[i].state = VOIDED; }
          }

          emblem.CreateWireEffect();
          return true;
        }
      }
    }
  }
  return false;
}

bool CardSelectionCust::CursorCancel() {
  if (isInFormSelect) {
    formSelectAnimator.SetAnimation("CLOSED");
    formCursorRow = 0;
    isInFormSelect = false;
    cursorPos = cursorRow = 0;
    return true;
  }

  // Unqueue all cards buckets
  if (selectCount <= 0) {
    selectCount = 0;
    return false;// nothing happened
  }

  selectQueue[--selectCount]->state = STAGED;

  if (selectCount == 0) {
    // Everything is selectable again
    for (int i = 0; i < cardCount; i++) {
      queue[i].state = STAGED;
    }

    emblem.UndoWireEffect();
    return true;
  }

  /*
    Compatible card states are built upon adding cards from the last available card states. 
    The only way to "revert" to the last compatible card states is to step through the already selected 
    card queue and build up the state again. 
  */


  for(int i = 0; i < selectCount; i++) {
    char code = selectQueue[i]->data->GetCode();

    for (int j = 0; j < cardCount; j++) {
      if (i > 0) {
        if (queue[j].state == VOIDED && code != queue[j].data->GetCode() - 1) continue; // already checked and not a PA sequence
      }

      if (queue[j].state == QUEUED) continue; // skip  

      char otherCode = queue[j].data->GetCode();

      bool isSameCard = (queue[j].data->GetShortName() == selectQueue[i]->data->GetShortName());

      if (code == WILDCARD || otherCode == WILDCARD || otherCode == code || isSameCard) { queue[j].state = STAGED; }
      else { queue[j].state = VOIDED; }
    }
  }

  emblem.UndoWireEffect();
  return true;
}

bool CardSelectionCust::CursorSelectOK()
{
  if (isInFormSelect || (cursorPos == 5 && cursorRow == 0)) return false;

  cursorPos = 5;
  cursorRow = 0;

  return true;
}

bool CardSelectionCust::IsOutOfView() {
  float bounds = 0;

  if (getPosition().x <= bounds) {
    setPosition(bounds, getPosition().y);
  }

  return (getPosition().x == bounds);
}

const bool CardSelectionCust::IsInView() {
  float bounds = custSprite.getTextureRect().width*2.f;

  // std::cout << "getPosition().x: " << getPosition().x << std::endl;

  if (getPosition().x >= bounds) {
    setPosition(bounds, getPosition().y);
  }

  return (getPosition().x == bounds);
}

bool CardSelectionCust::IsCardDescriptionTextBoxOpen()
{
  return cardDescriptionTextbox.IsOpen();
}

const bool CardSelectionCust::IsDarkCardSelected()
{
  return this->isDarkCardSelected && !this->IsHidden() && this->IsInView();
}

void CardSelectionCust::Move(sf::Vector2f delta) {
  setPosition(getPosition() + delta);
  IsInView();
}

bool CardSelectionCust::OpenCardDescription()
{
  if (isInFormSelect) return false;

  int index = cursorPos + (5 * cursorRow);

  if (!IsInView() || cardDescriptionTextbox.IsOpen() || 
    (cursorPos == 5 && cursorRow == 0) || (index >= cardCount)) return false;

  cardDescriptionTextbox.DescribeCard(queue[index].data);

  return true;
}

bool CardSelectionCust::ContinueCardDescription() {
  if (isInFormSelect) return false;
  if (!IsInView() || cardDescriptionTextbox.IsClosed()) return false;

  //cardDescriptionTextbox.Continue();

  return true;
}

bool CardSelectionCust::FastForwardCardDescription(double factor) {
  if (isInFormSelect) return false;
  if (!IsInView() || cardDescriptionTextbox.IsClosed()) return false;

  cardDescriptionTextbox.SetTextSpeed(factor);

  return true;
}

bool CardSelectionCust::CloseCardDescription() {
  if (isInFormSelect) return false;
  if (!IsInView() || cardDescriptionTextbox.IsClosed()) return false;

  cardDescriptionTextbox.Close();

  return true;
}

bool CardSelectionCust::CardDescriptionYes() {
  if (isInFormSelect) return false;
  if (!IsInView() || cardDescriptionTextbox.IsClosed()) return false;

  //return cardDescriptionTextbox.SelectYes();
  return true;

}

bool CardSelectionCust::CardDescriptionNo() {
  if (!IsInView() || cardDescriptionTextbox.IsClosed()) return false;

  //return cardDescriptionTextbox.SelectNo();
  return true;
}


bool CardSelectionCust::CardDescriptionConfirmQuestion() {
  if (isInFormSelect) return false;
  if (!IsInView() || cardDescriptionTextbox.IsClosed()) return false;

  // return cardDescriptionTextbox.ConfirmSelection();
  return true;
}

void CardSelectionCust::GetNextCards() {

  bool selectFirstDarkCard = true;

  for (int i = cardCount; i < cardCap; i++) {

    // The do-while loop ensures that only cards parsed successfully 
    // should be used in combat
    do {
      queue[i].data = folder->Next();

      if (!queue[i].data) {
        // nullptr is end of list
        return;
      }

      bool isDarkCard = queue[i].data->GetClass() == Battle::CardClass::dark;

      // This auto-selects the first dark card if visible
      if (selectFirstDarkCard && isDarkCard) {
        cursorPos = i % 5;
        cursorRow = i / 5;
        selectFirstDarkCard = false;
      }

      // NOTE: IsCardValid() will be used for scripted cards again... for now disable the check
    } while (false /*!CHIPLIB.IsCardValid(*queue[i].data)*/);

    queue[i].state = STAGED;

    cardCount++;
    perTurn--;

   if (perTurn == 0) return;
  }
}

void CardSelectionCust::SetPlayerFormOptions(const std::vector<PlayerFormMeta*> forms)
{
  for (auto&& f : forms) {
    this->forms.push_back(f);
    sf::Sprite ui;
    ui.setTexture(*TEXTURES.LoadTextureFromFile(f->GetUIPath()));
    ui.setScale(2.f, 2.f);
    formUI.push_back(ui);
  }
}

void CardSelectionCust::draw(sf::RenderTarget & target, sf::RenderStates states) const {
  if(IsHidden()) return;

  states.transform = getTransform();

  auto offset = -custSprite.getTextureRect().width*2.f; // TODO: this will be uneccessary once we use AddSprite() for all rendered items below
  custSprite.setPosition(-sf::Vector2f(custSprite.getTextureRect().width*2.f, 0));
  target.draw(custSprite, states);

  int index = cursorPos + (5 * cursorRow);

  // Make sure we are not selecting "OK" and we have a valid card highlighted
  if (index < cardCount && !(cursorPos == 5 && cursorRow == 0)) {
    switch (this->queue[index].data->GetClass()) {
    case Battle::CardClass::mega:
      custMegaCardOverlay.setPosition(custSprite.getPosition());
      target.draw(custMegaCardOverlay, states);
      break;
    case Battle::CardClass::giga:
      custGigaCardOverlay.setPosition(custSprite.getPosition());
      target.draw(custGigaCardOverlay, states);
      break;
    case Battle::CardClass::dark:
      custDarkCardOverlay.setPosition(custSprite.getPosition());
      target.draw(custDarkCardOverlay, states);
      break;
    }
  }

  auto lastEmblemPos = emblem.getPosition();
  emblem.setPosition(emblem.getPosition() + sf::Vector2f(offset, 0));
  target.draw(emblem, states);
  emblem.setPosition(lastEmblemPos);

  auto x = swoosh::ease::interpolate(frameElapsed*25, offset + (double)(2.f*(16.0f + ((float)cursorPos*16.0f))), (double)cursorSmall.getPosition().x);
  auto y = swoosh::ease::interpolate(frameElapsed*25, (double)(2.f*(113.f + ((float)cursorRow*24.f))), (double)cursorSmall.getPosition().y);

  cursorSmall.setPosition((float)x, (float)y); // TODO: Make this relative to cust instead of screen. hint: scene nodes

  int row = 0;
  for (int i = 0; i < cardCount; i++) {
    if (i > 4) {
      row = 1;
    }

    icon.setPosition(offset + 2.f*(9.0f + ((i%5)*16.0f)), 2.f*(105.f + (row*24.0f)) );
    icon.setTexture(WEBCLIENT.GetIconForCard(queue[i].data->GetUUID()));
    icon.SetShader(nullptr);

    if (queue[i].state == 0) {
      icon.SetShader(&greyscale);
      auto statesCopy = states;
        statesCopy.shader = &greyscale;

      target.draw(icon,statesCopy);

    } else if (queue[i].state == 1) {
      target.draw(icon, states);
    }

    smCodeLabel.setPosition(offset + 2.f*(14.0f + ((i % 5)*16.0f)), 2.f*(120.f + (row*24.0f)));

    char code = queue[i].data->GetCode();

    smCodeLabel.setString(code);
    target.draw(smCodeLabel, states);
  }

  icon.SetShader(nullptr);

  for (int i = 0; i < selectCount; i++) {
    icon.setPosition(offset + 2 * 97.f, 2.f*(25.0f + (i*16.0f)));
    icon.setTexture(WEBCLIENT.GetIconForCard((*selectQueue[i]).data->GetUUID()));

    cardLock.setPosition(offset + 2 * 93.f, 2.f*(23.0f + (i*16.0f)));
    target.draw(cardLock, states);
    target.draw(icon, states);
  }


  if ((cursorPos < 5 && cursorRow == 0) || (cursorPos < 3 && cursorRow == 1)) {
    // Draw the selected card info
    label.setFillColor(sf::Color::White);


    if (cursorPos + (5 * cursorRow) < cardCount) {
      // Draw the selected card card
      cardCard.setTexture(WEBCLIENT.GetImageForCard(queue[cursorPos+(5*cursorRow)].data->GetUUID()));

      auto lastPos = cardCard.getPosition();
      cardCard.setPosition(sf::Vector2f(offset, 0) + cardCard.getPosition());
      cardCard.SetShader(nullptr);

      if (!queue[cursorPos + (5 * cursorRow)].state) {
        cardCard.SetShader(&greyscale);

        auto statesCopy = states;
        statesCopy.shader = &greyscale;

        target.draw(cardCard, statesCopy);

      } else {
        target.draw(cardCard, states);
      }

      cardCard.setPosition(lastPos);

      label.setPosition(offset + 2.f*16.f, 16.f);
      label.setString(queue[cursorPos + (5 * cursorRow)].data->GetShortName());
      target.draw(label, states);

      // the order here is very important:
      if (queue[cursorPos + (5 * cursorRow)].data->GetDamage() > 0) {
        label.setString(std::to_string(queue[cursorPos + (5 * cursorRow)].data->GetDamage()));
        label.setOrigin(label.getLocalBounds().width+label.getLocalBounds().left, 0);
        label.setPosition(offset + 2.f*(70.f), 143.f);
        target.draw(label, states);
      }

      label.setOrigin(0, 0);
      label.setPosition(offset + 2.f*16.f, 143.f);
      label.setString(std::string() + queue[cursorPos + (5 * cursorRow)].data->GetCode());
      label.setFillColor(sf::Color(225, 180, 0));
      target.draw(label, states);

      int elementID = (int)(queue[cursorPos + (5 * cursorRow)].data->GetElement());
        
      auto elementRect = sf::IntRect(14 * elementID, 0, 14, 14);
      element.setTextureRect(elementRect);

      auto elementLastPos = element.getPosition();
      element.setPosition(element.getPosition() + sf::Vector2f(offset, 0));

      target.draw(element, states);

      element.setPosition(elementLastPos);
    }
    else {
      auto cardNoDataLastPos = cardNoData.getPosition();
      cardNoData.setPosition(cardNoData.getPosition() + sf::Vector2f(offset, 0));
      target.draw(cardNoData, states);
      cardNoData.setPosition(cardNoDataLastPos);
    }

    // Draw the small cursor
    if (!isInFormSelect) {
      target.draw(cursorSmall, states);
    }
  }
  else {
    auto cardSendDataLastPos = cardSendData.getPosition();
    cardSendData.setPosition(cardSendData.getPosition() + sf::Vector2f(offset, 0));
    target.draw(cardSendData, states);
    cardSendData.setPosition(cardSendDataLastPos);

    auto cursorBigLastPos = cursorBig.getPosition();
    cursorBig.setPosition(cursorBig.getPosition() + sf::Vector2f(offset, 0));
    target.draw(cursorBig, states);
    cursorBig.setPosition(cursorBigLastPos);
  }

  if (formUI.size()) {
    target.draw(formSelect, states);
  }

  // find out if we're in view (IsInView() is non-const qualified...)
  float bounds = custSprite.getTextureRect().width * 2.f;

  if (getPosition().x == bounds) {
    // Fade in a screen-wide shadow beneath the card if it is a dark card
    sf::RectangleShape screen(ENGINE.GetCamera()->GetView().getSize());
    screen.setFillColor(sf::Color(0, 0, 0, darkCardShadowAlpha * 255));
    target.draw(screen);
  }

  // draw the dark card in on top of it so it looks brighter
  if (isDarkCardSelected) {
    auto lastPos = cardCard.getPosition();
    cardCard.setPosition(sf::Vector2f(offset, 0) + cardCard.getPosition());
    target.draw(cardCard, states);
    cardCard.setPosition(lastPos);
  }

  if (isInFormSelect) {
    if (formSelectAnimator.GetAnimationString() == "OPEN") {
      int i = 0;
      auto offset = -custSprite.getTextureRect().width*2.f; // TODO: this will be uneccessary once we use AddSprite() for all rendered items below

      for (auto f : formUI) {
        formItemBG.setPosition(offset + 16.f, 16.f + float(i*32.0f));
        target.draw(formItemBG, states);

        f.setPosition(offset + 16.f, 16.f + float(i*32.0f));

        if (i != selectedForm && i != formCursorRow && selectedForm > -1) {
          auto greyscaleState = states;
          greyscaleState.shader = SHADERS.GetShader(ShaderType::GREYSCALE);
          target.draw(f, greyscaleState);
        }
        else if (i == selectedForm && selectedForm > -1) {
          auto aquaMarineState = states;
          aquaMarineState.shader = SHADERS.GetShader(ShaderType::COLORIZE);
          f.setColor(sf::Color(127,255,212));
          target.draw(f, aquaMarineState);
          f.setColor(sf::Color::White); // back to normal after draw to texture
        }
        else {
          // normal color
          target.draw(f, states);
        }

        i++;
      }

      // Draw cursor only if not flashing the screen
      if (formSelectQuitTimer <= 0.f) {
        target.draw(formCursor, states);
      }
    }
  }

  target.draw(cardDescriptionTextbox, states);

  SceneNode::draw(target, states);

  // Reveal the new form icon in the HUD after the flash
  if (selectedForm != -1 && formSelectQuitTimer <= 1.f/6.f) {
    auto f = formUI[selectedForm];
    auto rect = f.getTextureRect();
    f.setTextureRect(sf::IntRect(rect.left, rect.top, rect.width - 1, rect.height - 1));
    f.setPosition(sf::Vector2f(4, 36.f));
    target.draw(f, states);
    f.setTextureRect(rect);
  }

  if (isInFormSelect && formSelectQuitTimer <= 2.f / 6.0f) {
    auto delta = swoosh::ease::wideParabola(formSelectQuitTimer, 2.f / 6.f, 1.f);
    auto view = ENGINE.GetView();
    sf::RectangleShape screen(view.getSize());
    screen.setFillColor(sf::Color(255, 255, 255, int(delta * 255.f)));
    target.draw(screen, sf::RenderStates::Default);
  }
}


void CardSelectionCust::Update(float elapsed)
{
  if (IsHidden()) {
    canInteract = false;
    isDarkCardSelected = false;
    darkCardShadowAlpha = 0.f;
    return;
  }

  canInteract = true; // assume we can interact unless another flag says otherwise

  frameElapsed = (double)elapsed;

  if (!IsInView()) {
    darkCardShadowAlpha = 0.0f;
    isDarkCardSelected = false;
  }
  else {
    if (isDarkCardSelected) {
      darkCardShadowAlpha = std::min(darkCardShadowAlpha + elapsed, 0.3f);
    }
    else {
      darkCardShadowAlpha = std::max(darkCardShadowAlpha - elapsed, 0.0f);
    }
  }

  cursorSmallAnimator.Update(elapsed, cursorSmall);
  cursorBigAnimator.Update(elapsed, cursorBig);

  cardDescriptionTextbox.Update(elapsed);

  emblem.Update(elapsed);

  formCursorAnimator.Update(elapsed, formCursor.getSprite());
  formSelectAnimator.Update(elapsed, formSelect.getSprite());

  auto offset = -custSprite.getTextureRect().width*2.f; // TODO: this will be uneccessary once we use AddNode() for all rendered items below

  formCursor.setPosition(offset + 16.f, 12.f + float(formCursorRow*32.f));
  formSelect.setPosition(offset + 88.f, 194.f);

  if (isInFormSelect && formSelectQuitTimer > 0.f) {
    canInteract = false;

    formSelectQuitTimer -= elapsed;

    if (formSelectQuitTimer <= 0.f) {
      canInteract = true;
      isInFormSelect = false;
      playFormSound = false;
      thisFrameSelectedForm = -1;
    }
    else if (formSelectQuitTimer <= 1.f / 6.f) {
      formSelectAnimator.SetAnimation("CLOSED");
      cursorPos = cursorRow = formCursorRow = 0;
      if (!playFormSound) {
        playFormSound = true;
        AUDIO.Play(AudioType::PA_ADVANCE, AudioPriority::highest);
      }
    }
  }

#ifdef __ANDROID__
  sf::Vector2i touchPosition = sf::Touch::getPosition(0, *ENGINE.GetWindow());
  sf::Vector2f coords = ENGINE.GetWindow()->mapPixelToCoords(touchPosition, ENGINE.GetDefaultView());
  sf::Vector2i iCoords = sf::Vector2i((int)coords.x, (int)coords.y);
  touchPosition = iCoords;

  int row = 0;
  for (int i = 0; i < cardCount; i++) {
    if (i > 4) {
      row = 1;
    }
    if (IsInView()) {
        float offset = 0;//-custSprite.getTextureRect().width*2.f; // this will be uneccessary once we use AddNode() for all rendered items below

        icon.setPosition(offset + 2.f*(9.0f + ((float)(i%5)*16.0f)), 2.f*(105.f + (row*24.0f)) );

        sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID(queue[i].data->GetIconID());
        icon.setTextureRect(iconSubFrame);

        sf::FloatRect bounds = sf::FloatRect(icon.getPosition().x, icon.getPosition().y-1, 18, 18);

        bool touched = (touchPosition.x >= bounds.left && touchPosition.x <= bounds.left + bounds.width && touchPosition.y >= bounds.top && touchPosition.y <= bounds.top + bounds.height);

        if (touched && sf::Touch::isDown(0) && queue[i].state != 0) {
        cursorRow = row;
        cursorPos = i % 5;
        CursorAction();
      }
    }
  }

  sf::FloatRect deselect = sf::FloatRect();
  deselect.left = 2 * 97.f;
  deselect.top = 2.f*25.0f;
  deselect.width = 16;
  deselect.height = 2.f*(25.0f + (selectCount*8.0f));

  bool touched = (touchPosition.x >= deselect.left && touchPosition.x <= deselect.left + deselect.width && touchPosition.y >= deselect.top && touchPosition.y <= deselect.top + deselect.height);

  static bool tap = false;

  if(touched && sf::Touch::isDown(0) && !tap) {
      CursorCancel();
      tap = true;
  }

  if(!sf::Touch::isDown(0)) {
      tap = false;
  }

  cursorBig.setPosition(sf::Vector2f(2.f*104.f, 2.f*110.f));
  sf::FloatRect OK = sf::FloatRect(cursorBig.getPosition().x-20.0f, cursorBig.getPosition().y-5.0f, cursorBig.getGlobalBounds().width, cursorBig.getGlobalBounds().height);
  touched = (touchPosition.x >= OK.left && touchPosition.x <= OK.left + OK.width && touchPosition.y >= OK.top && touchPosition.y <= OK.top + OK.height);

  if(touched && sf::Touch::isDown(0) && !tap) {
    cursorRow = 0;
    cursorPos = 5;

    areCardsReady = true;
    tap = true;
 }

#endif

  if (cardCount > 0) {
    // If OK button is highlighted, we are not selecting a dark card
    // If we are in form select, we are no selecting a dark card
    if (cursorRow == 0 && cursorPos == 5 || isInFormSelect) {
      isDarkCardSelected = false;
    }
    else {
      // Otherwise check if we are highlighting a dark card 
      int index = cursorPos + (5 * cursorRow);
      isDarkCardSelected = this->queue[index].data->GetClass() == Battle::CardClass::dark;
    }
  } 

  isInView = IsInView();
}

Battle::Card** CardSelectionCust::GetCards() {
  // Allocate selected cards list
  selectedCards = (Battle::Card**)malloc(sizeof(Battle::Card*)*selectCount);

  // Point to selected queue
  for (int i = 0; i < selectCount; i++) {
    selectedCards[i] = (*(selectQueue[i])).data;
  }

  return selectedCards;
}

void CardSelectionCust::ClearCards() {
  if (areCardsReady) {
    for (int i = 0; i < selectCount; i++) {
      selectedCards[i] = nullptr; // point away
      (*(selectQueue[i])).data = nullptr;
    }
  
    // Deleted the selected cards array
    if (selectCount > 0) {
      delete[] selectedCards;
    }
  }

  // Restructure queue
  // Line up all non-null buckets consequtively
  // Reset bucket state to STAGED
  for (int i = 0; i < cardCount; i++) {
    queue[i].state = STAGED;
    int next = i;
    while (!queue[i].data && next + 1 < cardCount) {
      queue[i].data = queue[next + 1].data;
      queue[next + 1].data = nullptr;
      next++;
    }
  }

  cardCount = cardCount - selectCount;
  selectCount = 0;
}

const int CardSelectionCust::GetCardCount() {
  return selectCount;
}

const int CardSelectionCust::GetSelectedFormIndex()
{
  return selectedForm;
}

const bool CardSelectionCust::SelectedNewForm()
{
  return (thisFrameSelectedForm != -1);
}

bool CardSelectionCust::CanInteract()
{
  return canInteract;
}

void CardSelectionCust::ResetState() {
  ClearCards();

  cursorPos = formCursorRow = 0;
  areCardsReady = false;
  isInFormSelect = false;
  thisFrameSelectedForm = -1;
}

bool CardSelectionCust::AreCardsReady() {
  return areCardsReady;
}
