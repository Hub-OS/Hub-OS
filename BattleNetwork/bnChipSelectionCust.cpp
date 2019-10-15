#include "bnChipSelectionCust.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnInputManager.h"
#include "bnChipLibrary.h"

#define WILDCARD '*'
#define VOIDED 0
#define STAGED 1
#define QUEUED 2

ChipSelectionCust::ChipSelectionCust(ChipFolder* _folder, int cap, int perTurn) :
  perTurn(perTurn),
  greyscale(*SHADERS.GetShader(ShaderType::GREYSCALE)),
  chipDescriptionTextbox(sf::Vector2f(4, 255)),
  isInView(false),
  isInFormSelect(false),
  playFormSound(false),
  canInteract(true)
{
  frameElapsed = 1;
  folder = _folder;
  cap = std::min(cap, 8);
  chipCap = cap;
  queue = new Bucket[chipCap];
  selectQueue = new Bucket*[chipCap];

  chipCount = selectCount = cursorPos = cursorRow = 0;

  emblem.setScale(2.f, 2.f);
  emblem.setPosition(194.0f, 14.0f);

  custSprite = sf::Sprite(*TEXTURES.GetTexture(TextureType::CHIP_SELECT_MENU));
  custSprite.setScale(2.f, 2.f);
  custSprite.setPosition(-custSprite.getTextureRect().width*2.f, 0);

  //this->AddSprite(custSprite);

  icon.setTexture(*TEXTURES.GetTexture(CHIP_ICONS));
  icon.setScale(sf::Vector2f(2.f, 2.f));

  sf::Texture* elementTexture = TEXTURES.GetTexture(TextureType::ELEMENT_ICON);
  element.setTexture(*elementTexture);
  element.setScale(2.f, 2.f);
  element.setPosition(2.f*25.f, 146.f);

  cursorSmall = sf::Sprite(*TEXTURES.GetTexture(TextureType::CHIP_CURSOR_SMALL));
  cursorSmall.setScale(sf::Vector2f(2.f, 2.f));

  cursorBig = sf::Sprite(*TEXTURES.GetTexture(TextureType::CHIP_CURSOR_BIG));
  cursorBig.setScale(sf::Vector2f(2.f, 2.f));

  // never moves
  cursorBig.setPosition(sf::Vector2f(2.f*104.f, 2.f*122.f));

  chipLock = sf::Sprite(LOAD_TEXTURE(CHIP_LOCK));
  chipLock.setScale(sf::Vector2f(2.f, 2.f));
  
  sf::Texture* card = TEXTURES.GetTexture(TextureType::CHIP_CARDS);
  chipCard.setTexture(*card);
  chipCard.setScale(2.f, 2.f);
  chipCard.setPosition(2.f*16.f, 48.f);

  sf::Texture* nodata = TEXTURES.GetTexture(TextureType::CHIP_NODATA);
  chipNoData.setTexture(*nodata);
  chipNoData.setScale(2.f, 2.f);
  chipNoData.setPosition(2.f*16.f, 48.f);

  sf::Texture* senddata = TEXTURES.GetTexture(TextureType::CHIP_SENDDATA);
  chipSendData.setTexture(*senddata);
  chipSendData.setScale(2.f, 2.f);
  chipSendData.setPosition(2.f*16.f, 48.f);

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

  formItemBG.setTexture(LOAD_TEXTURE(CUST_FORM_ITEM_BG));
  formItemBG.setScale(2.f, 2.f);

  formSelect.setTexture(LOAD_TEXTURE(CUST_FORM_SELECT));
  formCursor.setTexture(LOAD_TEXTURE(CUST_FORM_CURSOR));

  formSelect.setScale(2.f, 2.f);
  formCursor.setScale(2.f, 2.f);

  //this->setScale(0.5f, 0.5); // testing transforms
}


ChipSelectionCust::~ChipSelectionCust() {
  ClearChips();

  for (int i = 0; i < chipCount; i++) {
    //selectQueue[i] = nullptr;
    // delete queue[i].data;
  }

  if (chipCount > 0) {
    delete[] queue;
    delete[] selectQueue;
  }

  chipCount = 0;

  delete labelFont;
  delete codeFont;
  delete folder;
}

bool ChipSelectionCust::CursorUp() {
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
          this->formSelectAnimator << "OPEN";
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

bool ChipSelectionCust::CursorDown() {
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

bool ChipSelectionCust::CursorRight() {
  if (isInFormSelect) return false;

  if (++cursorPos > 2 && cursorRow == 1) {
    cursorPos = 0;
  }
  else if (cursorPos > 5 && cursorRow == 0) {
    cursorPos = 0;
  }

  return true;
}

bool ChipSelectionCust::CursorLeft() {
  if (isInFormSelect) return false;

  if (--cursorPos < 0 && cursorRow == 1) {
    cursorPos = 2;
  }
  else if (cursorPos < 0 && cursorRow == 0) {
    cursorPos = 5;
  }

  return true;
}

bool ChipSelectionCust::CursorAction() {
  if (isInFormSelect) {
    auto res = true;

    thisFrameSelectedForm = formCursorRow;

    if (thisFrameSelectedForm == selectedForm) {
      res = true;
      selectedForm = thisFrameSelectedForm = -1; // no change
    }
    else {
      formSelectQuitTimer = 3.0f/6.0f;
      selectedForm = thisFrameSelectedForm;
    }
    return res;
  }

  // Should never happen but just in case
  if (selectCount > 5) {
    return false;
  }

  if (cursorPos == 5 && cursorRow == 0) {
    // End chip select
    areChipsReady = true;
  } else {
    // Does chip exist 
    if (cursorPos + (5 * cursorRow) < chipCount) {
      // Queue this chip if not selected
      if (queue[cursorPos + (5 * cursorRow)].state == STAGED) {
        selectQueue[selectCount++] = &queue[cursorPos + (5 * cursorRow)];
        queue[cursorPos + (5 * cursorRow)].state = QUEUED;

        // We can only upload 5 chips to navi...
        if (selectCount == 5) {
          for (int i = 0; i < chipCount; i++) {
            if (queue[i].state == QUEUED) continue;

            queue[i].state = VOIDED;
          }

          emblem.CreateWireEffect();
          return true;
        }
        else {
          // Otherwise display the remaining compatible chips...
          // Check chip code. If other chips are not compatible, set their bucket state flag to 0
          char code = queue[cursorPos + (5 * cursorRow)].data->GetCode();

          for (int i = 0; i < chipCount; i++) {
            if (i == cursorPos + (5 * cursorRow) || (queue[i].state == VOIDED) || queue[i].state == QUEUED) continue;
            char otherCode = queue[i].data->GetCode();
            bool isSameChip = (queue[i].data->GetShortName() == queue[cursorPos + (5 * cursorRow)].data->GetShortName());
            if (code == WILDCARD || otherCode == WILDCARD || otherCode == code || isSameChip) { queue[i].state = STAGED; }
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

bool ChipSelectionCust::CursorCancel() {
  if (isInFormSelect) {
    formSelectAnimator.SetAnimation("CLOSED");
    formCursorRow = 0;
    isInFormSelect = false;
    cursorPos = cursorRow = 0;
    return true;
  }

  // Unqueue all chips buckets
  if (selectCount <= 0) {
    selectCount = 0;
    return false;// nothing happened
  }

  selectQueue[--selectCount]->state = STAGED;

  if (selectCount == 0) {
    // Everything is selectable again
    for (int i = 0; i < chipCount; i++) {
      queue[i].state = STAGED;
    }

    emblem.UndoWireEffect();
    return true;
  }

  /*
    Compatible chip states are built upon adding chips from the last available chip states. 
    The only way to "revert" to the last compatible chip states is to step through the already selected 
    chip queue and build up the state again. 
  */


  for(int i = 0; i < selectCount; i++) {
    char code = selectQueue[i]->data->GetCode();

    for (int j = 0; j < chipCount; j++) {
      if (i > 0) {
        if (queue[j].state == VOIDED && code != queue[j].data->GetCode() - 1) continue; // already checked and not a PA sequence
      }

      if (queue[j].state == QUEUED) continue; // skip  

      char otherCode = queue[j].data->GetCode();

      bool isSameChip = (queue[j].data->GetShortName() == selectQueue[i]->data->GetShortName());

      if (code == WILDCARD || otherCode == WILDCARD || otherCode == code || isSameChip) { queue[j].state = STAGED; }
      else { queue[j].state = VOIDED; }
    }
  }

  emblem.UndoWireEffect();
  return true;
}

bool ChipSelectionCust::IsOutOfView() {
  float bounds = 0;

  if (this->getPosition().x <= bounds) {
    this->setPosition(bounds, this->getPosition().y);
  }

  return (this->getPosition().x == bounds);
}

const bool ChipSelectionCust::IsInView() {
  float bounds = custSprite.getTextureRect().width*2.f;

  // std::cout << "this->getPosition().x: " << this->getPosition().x << std::endl;

  if (this->getPosition().x >= bounds) {
    this->setPosition(bounds, this->getPosition().y);
  }

  return (this->getPosition().x == bounds);
}

bool ChipSelectionCust::IsChipDescriptionTextBoxOpen()
{
  return chipDescriptionTextbox.IsOpen();
}

void ChipSelectionCust::Move(sf::Vector2f delta) {
  this->setPosition(this->getPosition() + delta);
  this->IsInView();
}

bool ChipSelectionCust::OpenChipDescription()
{
  if (isInFormSelect) return false;

  int index = cursorPos + (5 * cursorRow);

  if (!IsInView() || chipDescriptionTextbox.IsOpen() || 
    (cursorPos == 5 && cursorRow == 0) || (index >= chipCount)) return false;

  chipDescriptionTextbox.DescribeChip(queue[index].data);

  return true;
}

bool ChipSelectionCust::ContinueChipDescription() {
  if (isInFormSelect) return false;
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  //chipDescriptionTextbox.Continue();

  return true;
}

bool ChipSelectionCust::FastForwardChipDescription(double factor) {
  if (isInFormSelect) return false;
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  chipDescriptionTextbox.SetTextSpeed(factor);

  return true;
}

bool ChipSelectionCust::CloseChipDescription() {
  if (isInFormSelect) return false;
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  chipDescriptionTextbox.Close();

  return true;
}

bool ChipSelectionCust::ChipDescriptionYes() {
  if (isInFormSelect) return false;
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  //return chipDescriptionTextbox.SelectYes();
  return true;

}

bool ChipSelectionCust::ChipDescriptionNo() {
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  //return chipDescriptionTextbox.SelectNo();
  return true;
}


bool ChipSelectionCust::ChipDescriptionConfirmQuestion() {
  if (isInFormSelect) return false;
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  // return chipDescriptionTextbox.ConfirmSelection();
  return true;
}

void ChipSelectionCust::GetNextChips() {
  for (int i = chipCount; i < chipCap; i++) {
    do {
      queue[i].data = folder->Next();

      if (!queue[i].data) {
        // nullptr is end of list
        return;
      }
    } while (!CHIPLIB.IsChipValid(*queue[i].data)); // Only chips parsed successfully should be used in combat

    queue[i].state = STAGED;

    chipCount++;
    perTurn--;

   if (perTurn == 0) return;
  }
}

void ChipSelectionCust::SetPlayerFormOptions(const std::vector<PlayerFormMeta*> forms)
{
  for (auto f : forms) {
    this->forms.push_back(f);
    sf::Sprite ui;
    ui.setTexture(*TEXTURES.LoadTextureFromFile(f->GetUIPath()));
    ui.setScale(2.f, 2.f);
    formUI.push_back(ui);
  }
}

void ChipSelectionCust::draw(sf::RenderTarget & target, sf::RenderStates states) const {
  if(this->IsHidden()) return;

  // combine the parent transform with the node's one
  sf::Transform combinedTransform = this->getTransform();

  states.transform = combinedTransform;

  auto offset = -custSprite.getTextureRect().width*2.f; // TODO: this will be uneccessary once we use this->AddSprite() for all rendered items below
  custSprite.setPosition(-sf::Vector2f(custSprite.getTextureRect().width*2.f, 0));
  target.draw(custSprite, states);


  //if (isInView) {
    auto lastEmblemPos = emblem.getPosition();
    emblem.setPosition(emblem.getPosition() + sf::Vector2f(offset, 0));
    target.draw(emblem, states);
    emblem.setPosition(lastEmblemPos);

    auto x = swoosh::ease::interpolate(frameElapsed*25, offset + (double)(2.f*(16.0f + (cursorPos*16.0f))), (double)cursorSmall.getPosition().x);
    auto y = swoosh::ease::interpolate(frameElapsed*25, (double)(2.f*(113.f + (cursorRow*24.f))), (double)cursorSmall.getPosition().y);

    cursorSmall.setPosition((float)x, (float)y); // TODO: Make this relative to cust instead of screen. hint: scene nodes

    int row = 0;
    for (int i = 0; i < chipCount; i++) {
      if (i > 4) {
        row = 1;
      }

      icon.setPosition(offset + 2.f*(9.0f + ((i%5)*16.0f)), 2.f*(105.f + (row*24.0f)) );
      sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID(queue[i].data->GetIconID());
      icon.setTextureRect(iconSubFrame);
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

      if (code == WILDCARD && code != '*') {
        code = '*';
      }

      smCodeLabel.setString(code);
      target.draw(smCodeLabel, states);
    }

    icon.SetShader(nullptr);

    for (int i = 0; i < selectCount; i++) {
      icon.setPosition(offset + 2 * 97.f, 2.f*(25.0f + (i*16.0f)));
      sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID((*selectQueue[i]).data->GetIconID());
      icon.setTextureRect(iconSubFrame);

      chipLock.setPosition(offset + 2 * 93.f, 2.f*(23.0f + (i*16.0f)));
      target.draw(chipLock, states);
      target.draw(icon, states);
    }


    if ((cursorPos < 5 && cursorRow == 0) || (cursorPos < 3 && cursorRow == 1)) {
      // Draw the selected chip info
      label.setFillColor(sf::Color::White);

      if (cursorPos + (5 * cursorRow) < chipCount) {
        // Draw the selected chip card
        sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID(queue[cursorPos+(5*cursorRow)].data->GetID());

        auto lastPos = chipCard.getPosition();
        chipCard.setPosition(sf::Vector2f(offset, 0) + chipCard.getPosition());
        chipCard.setTextureRect(cardSubFrame);

        chipCard.SetShader(nullptr);

        if (!queue[cursorPos + (5 * cursorRow)].state) {
          chipCard.SetShader(&greyscale);

          auto statesCopy = states;
          statesCopy.shader = &greyscale;

          target.draw(chipCard, statesCopy);

        } else {
          target.draw(chipCard, states);
        }

        chipCard.setPosition(lastPos);

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
        auto chipNoDataLastPos = chipNoData.getPosition();
        chipNoData.setPosition(chipNoData.getPosition() + sf::Vector2f(offset, 0));
        target.draw(chipNoData, states);
        chipNoData.setPosition(chipNoDataLastPos);
      }

      // Draw the small cursor
      if (!isInFormSelect) {
        target.draw(cursorSmall, states);
      }
    }
    else {
      auto chipSendDataLastPos = chipSendData.getPosition();
      chipSendData.setPosition(chipSendData.getPosition() + sf::Vector2f(offset, 0));
      target.draw(chipSendData, states);
      chipSendData.setPosition(chipSendDataLastPos);

      auto cursorBigLastPos = cursorBig.getPosition();
      cursorBig.setPosition(cursorBig.getPosition() + sf::Vector2f(offset, 0));
      target.draw(cursorBig, states);
      cursorBig.setPosition(cursorBigLastPos);
    }
  //}

    target.draw(formSelect, states);

  if (isInFormSelect) {
    if (formSelectAnimator.GetAnimationString() == "OPEN") {
      int i = 0;
      auto offset = -custSprite.getTextureRect().width*2.f; // TODO: this will be uneccessary once we use this->AddSprite() for all rendered items below

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

  target.draw(chipDescriptionTextbox, states);

  SceneNode::draw(target, states);

  // Reveal the new form icon in the HUD after the flash
  if (selectedForm != -1 && formSelectQuitTimer <= 1.f/6.f) {
    auto f = formUI[selectedForm];
    auto rect = f.getTextureRect();
    f.setTextureRect(sf::Rect(rect.left, rect.top, rect.width - 1, rect.height - 1));
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


void ChipSelectionCust::Update(float elapsed)
{
  if (this->IsHidden()) {
    canInteract = false;
    return;
  }

  canInteract = true; // assume we can interact unless another flag says otherwise

  frameElapsed = (double)elapsed;

  cursorSmallAnimator.Update(elapsed, cursorSmall);
  cursorBigAnimator.Update(elapsed, cursorBig);

  chipDescriptionTextbox.Update(elapsed);

  emblem.Update(elapsed);

  formCursorAnimator.Update(elapsed, formCursor);
  formSelectAnimator.Update(elapsed, formSelect);

  auto offset = -custSprite.getTextureRect().width*2.f; // TODO: this will be uneccessary once we use this->AddNode() for all rendered items below

  formCursor.setPosition(offset + 16.f, 12.f + float(formCursorRow*32.f));
  formSelect.setPosition(offset + 88.f, 194.f);

  if (isInFormSelect && formSelectQuitTimer > 0.f) {
    canInteract = false;

    formSelectQuitTimer -= elapsed;

    if (formSelectQuitTimer <= 0.f) {
      canInteract = true;
      isInFormSelect = false;
      playFormSound = false;
    } else if (formSelectQuitTimer <= 1.f / 6.f) {
      formSelectAnimator.SetAnimation("CLOSED");
      cursorPos = cursorRow = formCursorRow = 0;
      if (!playFormSound) {
        playFormSound = true;
        AUDIO.Play(AudioType::PA_ADVANCE);
      }
    }
  }

#ifdef __ANDROID__
  sf::Vector2i touchPosition = sf::Touch::getPosition(0, *ENGINE.GetWindow());
  sf::Vector2f coords = ENGINE.GetWindow()->mapPixelToCoords(touchPosition, ENGINE.GetDefaultView());
  sf::Vector2i iCoords = sf::Vector2i((int)coords.x, (int)coords.y);
  touchPosition = iCoords;

  int row = 0;
  for (int i = 0; i < chipCount; i++) {
    if (i > 4) {
      row = 1;
    }
    if (IsInView()) {
        float offset = 0;//-custSprite.getTextureRect().width*2.f; // this will be uneccessary once we use this->AddNode() for all rendered items below

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

    areChipsReady = true;
    tap = true;
 }

#endif

  this->isInView = IsInView();
}

Chip** ChipSelectionCust::GetChips() {
  // Allocate selected chips list
  selectedChips = (Chip**)malloc(sizeof(Chip*)*selectCount);

  // Point to selected queue
  for (int i = 0; i < selectCount; i++) {
    selectedChips[i] = (*(selectQueue[i])).data;
  }

  return selectedChips;
}

void ChipSelectionCust::ClearChips() {
  if (areChipsReady) {
    for (int i = 0; i < selectCount; i++) {
      selectedChips[i] = nullptr; // point away
      (*(selectQueue[i])).data = nullptr;
    }
  
    // Deleted the selected chips array
    if (selectCount > 0) {
      delete[] selectedChips;
    }
  }

  // Restructure queue
  // Line up all non-null buckets consequtively
  // Reset bucket state to STAGED
  for (int i = 0; i < chipCount; i++) {
    queue[i].state = STAGED;
    int next = i;
    while (!queue[i].data && next + 1 < chipCount) {
      queue[i].data = queue[next + 1].data;
      queue[next + 1].data = nullptr;
      next++;
    }
  }

  chipCount = chipCount - selectCount;
  selectCount = 0;
}

const int ChipSelectionCust::GetChipCount() {
  return selectCount;
}

const int ChipSelectionCust::GetSelectedFormIndex()
{
  return selectedForm;
}

const bool ChipSelectionCust::SelectedNewForm()
{
  return (thisFrameSelectedForm != -1);
}

bool ChipSelectionCust::CanInteract()
{
  return canInteract;
}

void ChipSelectionCust::ResetState() {
  ClearChips();

  cursorPos = formCursorRow = 0;
  areChipsReady = false;
  isInFormSelect = false;
  thisFrameSelectedForm = -1;
}

bool ChipSelectionCust::AreChipsReady() {
  return areChipsReady;
}
