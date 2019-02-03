#include "bnChipSelectionCust.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnInputManager.h"
#include "bnChipLibrary.h"

#define WILDCARD '*'
#define VOIDED 0
#define STAGED  1
#define QUEUED 2

ChipSelectionCust::ChipSelectionCust(ChipFolder* _folder, int cap, int perTurn) :
  perTurn(perTurn),
  greyscale(*SHADERS.GetShader(ShaderType::GREYSCALE)),
  chipDescriptionTextbox(sf::Vector2f(4, 255))
{
  frameElapsed = 1;
  folder = _folder;
  cap = std::min(cap, 8);
  chipCap = cap;
  queue = new Bucket[chipCap];
  selectQueue = new Bucket*[chipCap];

  chipCount = selectCount = cursorPos = cursorRow = 0;

  custSprite = sf::Sprite(*TEXTURES.GetTexture(TextureType::CHIP_SELECT_MENU));
  custSprite.setScale(2.f, 2.f);
  custSprite.setPosition(-custSprite.getTextureRect().width*2.f, 0);

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
  cursorSmallAnimator << Animate::Mode::Loop;

  cursorBigAnimator = Animation("resources/ui/cursor_big.animation");
  cursorBigAnimator.Reload();
  cursorBigAnimator.SetAnimation("BLINK");
  cursorBigAnimator << Animate::Mode::Loop;
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
  if (--cursorRow < 0) {
    cursorRow = 0;
    return false;
  }

  return true;
}

bool ChipSelectionCust::CursorDown() {
  if (++cursorRow > 1) {
    cursorRow = 1;

    return false;
  }

  if (cursorPos > 2) {
    cursorPos = 0;
  }

  return true;
}

bool ChipSelectionCust::CursorRight() {
  if (++cursorPos > 2 && cursorRow == 1) {
    cursorPos = 0;
  }
  else if (cursorPos > 5 && cursorRow == 0) {
    cursorPos = 0;
  }

  return true;
}

bool ChipSelectionCust::CursorLeft() {
  if (--cursorPos < 0 && cursorRow == 1) {
    cursorPos = 2;
  }
  else if (cursorPos < 0 && cursorRow == 0) {
    cursorPos = 5;
  }

  return true;
}

bool ChipSelectionCust::CursorAction() {
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

          return true;
        }
      }
    }
  }
  return false;
}

bool ChipSelectionCust::CursorCancel() {
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

  return true;
}

bool ChipSelectionCust::IsOutOfView() {
  float bounds = -custSprite.getTextureRect().width*2.f;

  if (custSprite.getPosition().x <= bounds) {
    custSprite.setPosition(bounds, custSprite.getPosition().y);
  }

  return (custSprite.getPosition().x == bounds);
}

bool ChipSelectionCust::IsInView() {
  float bounds = 0;

  if (custSprite.getPosition().x >= bounds) {
    custSprite.setPosition(bounds, custSprite.getPosition().y);
  }

  return (custSprite.getPosition().x == bounds);
}

bool ChipSelectionCust::IsChipDescriptionTextBoxOpen()
{
  return chipDescriptionTextbox.IsOpen();
}

void ChipSelectionCust::Move(sf::Vector2f delta) {
  custSprite.setPosition(custSprite.getPosition() + delta);
}

bool ChipSelectionCust::OpenChipDescription()
{
  int index = cursorPos + (5 * cursorRow);

  if (!IsInView() || chipDescriptionTextbox.IsOpen() || 
    (cursorPos == 5 && cursorRow == 0) || (index >= chipCount)) return false;

  chipDescriptionTextbox.DescribeChip(queue[index].data);

  return true;
}

bool ChipSelectionCust::ContinueChipDescription() {
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  chipDescriptionTextbox.Continue();

  return true;
}

bool ChipSelectionCust::FastForwardChipDescription(double factor) {
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  chipDescriptionTextbox.SetTextSpeed(factor);

  return true;
}

bool ChipSelectionCust::CloseChipDescription() {
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  chipDescriptionTextbox.Close();

  return true;
}

bool ChipSelectionCust::ChipDescriptionYes() {
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  return chipDescriptionTextbox.SelectYes();
}

bool ChipSelectionCust::ChipDescriptionNo() {
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  return chipDescriptionTextbox.SelectNo();
}


bool ChipSelectionCust::ChipDescriptionConfirmQuestion() {
  if (!IsInView() || chipDescriptionTextbox.IsClosed()) return false;

  return chipDescriptionTextbox.ConfirmSelection();
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

void ChipSelectionCust::Draw() {
  ENGINE.Draw(custSprite, false);

  if (IsInView()) {
    auto x = swoosh::ease::interpolate(frameElapsed*25, (double)(2.f*(16.0f + (cursorPos*16.0f))), (double)cursorSmall.getPosition().x);
    auto y = swoosh::ease::interpolate(frameElapsed*25, (double)(2.f*(113.f + (cursorRow*24.f))), (double)cursorSmall.getPosition().y);

    cursorSmall.setPosition((float)x, (float)y); // TODO: Make this relative to cust instead of screen. hint: scene nodes

    int row = 0;
    for (int i = 0; i < chipCount; i++) {
      if (i > 4) {
        row = 1;
      }

      icon.setPosition(2.f*(9.0f + ((i%5)*16.0f)), 2.f*(105.f + (row*24.0f)) );
      sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID(queue[i].data->GetIconID());
      icon.setTextureRect(iconSubFrame);
      icon.SetShader(nullptr);

      if (queue[i].state == 0) {
        icon.SetShader(&greyscale);
        ENGINE.Draw(&icon);
      } else if (queue[i].state == 1) {
        ENGINE.Draw(icon, false);
      }

      smCodeLabel.setPosition(2.f*(14.0f + ((i % 5)*16.0f)), 2.f*(120.f + (row*24.0f)));

      char code = queue[i].data->GetCode();

      if (code == WILDCARD && code != '*') {
        code = '*';
      }

      smCodeLabel.setString(code);
      ENGINE.Draw(smCodeLabel, false);
    }

    icon.SetShader(nullptr);

    for (int i = 0; i < selectCount; i++) {
      icon.setPosition(2 * 97.f, 2.f*(25.0f + (i*16.0f)));
      sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID((*selectQueue[i]).data->GetIconID());
      icon.setTextureRect(iconSubFrame);

      chipLock.setPosition(2 * 93.f, 2.f*(23.0f + (i*16.0f)));
      ENGINE.Draw(chipLock, false);
      ENGINE.Draw(icon, false);
    }


    if (cursorPos < 5 && cursorRow == 0 || cursorPos < 3 && cursorRow == 1) {
      // Draw the selected chip info
      label.setFillColor(sf::Color::White);

      if (cursorPos + (5 * cursorRow) < chipCount) {
        // Draw the selected chip card
        sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID(queue[cursorPos+(5*cursorRow)].data->GetID());
        chipCard.setTextureRect(cardSubFrame);

        chipCard.SetShader(nullptr);

        if (!queue[cursorPos + (5 * cursorRow)].state) {
          chipCard.SetShader(&greyscale);
          ENGINE.Draw((LayeredDrawable*)&chipCard);
        } else {
          ENGINE.Draw(chipCard, false);
        }

        label.setPosition(2.f*16.f, 16.f);
        label.setString(queue[cursorPos + (5 * cursorRow)].data->GetShortName());
        ENGINE.Draw(label, false);

        // the order here is very important:
        if (queue[cursorPos + (5 * cursorRow)].data->GetDamage() > 0) {
          label.setString(std::to_string(queue[cursorPos + (5 * cursorRow)].data->GetDamage()));
          label.setOrigin(label.getLocalBounds().width+label.getLocalBounds().left, 0);
          label.setPosition(2.f*(70.f), 143.f);
          ENGINE.Draw(label, false);
        }

        label.setOrigin(0, 0);
        label.setPosition(2.f*16.f, 143.f);
        label.setString(std::string() + queue[cursorPos + (5 * cursorRow)].data->GetCode());
        label.setFillColor(sf::Color(225, 180, 0));
        ENGINE.Draw(label, false);

        int offset = (int)(queue[cursorPos + (5 * cursorRow)].data->GetElement());
        element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
        ENGINE.Draw(element, false);
      }
      else {
        ENGINE.Draw(chipNoData, false);
      }

      // Draw the small cursor
      ENGINE.Draw(cursorSmall, false);
    }
    else {
      ENGINE.Draw(chipSendData, false);
      ENGINE.Draw(cursorBig, false);
    }
  }

  ENGINE.Draw(chipDescriptionTextbox);
}

void ChipSelectionCust::Update(float elapsed)
{
  frameElapsed = (double)elapsed;

  cursorSmallAnimator.Update(elapsed, &cursorSmall);
  cursorBigAnimator.Update(elapsed, &cursorBig);

  chipDescriptionTextbox.Update(elapsed);
}

Chip** ChipSelectionCust::GetChips() {
  selectedChips = (Chip**)malloc(sizeof(Chip*)*selectCount);

  for (int i = 0; i < selectCount; i++) {
    selectedChips[i] = (*(selectQueue[i])).data;
  }

  return selectedChips;
}

void ChipSelectionCust::ClearChips() {
  if (areChipsReady) {
    for (int i = 0; i < selectCount; i++) {
      selectedChips[i] = nullptr; // point away
      //delete (*(selectQueue[i])).data;
      (*(selectQueue[i])).data = nullptr;
    }

    if (selectCount > 0) {
      delete[] selectedChips;
    }
  }

  // Restructure queue
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

void ChipSelectionCust::ResetState() {
  ClearChips();

  cursorPos = 0;
  areChipsReady = false;
}

bool ChipSelectionCust::AreChipsReady() {
  return areChipsReady;
}
