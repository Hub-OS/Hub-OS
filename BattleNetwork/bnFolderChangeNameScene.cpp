#include "bnFolderChangeNameScene.h"
#include "bnInputManager.h"
#include "Segues\BlackWashFade.h"
#include <Swoosh/ActivityController.h>

#define UPPERCASE 0
#define LOWERCASE 1
#define COMMANDS  2

void FolderChangeNameScene::ExecuteAction(size_t table, size_t x, size_t y)
{
  auto entry = this->table[table][y][x];

  if (entry == "NEXT") {
    DoNEXT();
  }
  else if (entry == "BACK") {
    DoBACK();
  }
  else if (entry == "OK") {
    DoOK();
  }
  else if (entry == "END") {
    DoEND();
  }
  else { // it's a letter entry
    if (letterPos + 1 == 9) { // denotes end of input, reject
      AUDIO.Play(AudioType::CHIP_ERROR);
    }
    else { // not end of input, add latter, and increase letterPos
      name[letterPos++] = entry[0];
      letterPos = std::min(letterPos, 8);

      AUDIO.Play(AudioType::CHIP_CHOOSE);
    }
  }
}

void FolderChangeNameScene::DoBACK()
{
  letterPos = std::max(0, letterPos - 1);

}

void FolderChangeNameScene::DoNEXT()
{
  letterPos = std::min(letterPos + 1, 8);

}

void FolderChangeNameScene::DoOK()
{
  // Take '_' out of names
  auto copy = name;
  std::replace(copy.begin(), copy.end(), '_', ' ');

  // Prompt question

  // Save results
  folderName = copy;

  // Finish
  DoEND();
}

void FolderChangeNameScene::DoEND()
{
  using namespace swoosh::intent;
  using effect = segue<BlackWashFade, swoosh::intent::milli<500>>;
  getController().queuePop<effect>();
  AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
  leave = true;
}

FolderChangeNameScene::FolderChangeNameScene(swoosh::ActivityController& controller, std::string& folderName) : swoosh::Activity(&controller), folderName(folderName) {
  
  leave = true;
  // folder menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(FOLDER_CHANGE_NAME_BG));
  bg.setScale(2.f, 2.f);

  cursorPieceLeft = sf::Sprite(LOAD_TEXTURE(LETTER_CURSOR));
  cursorPieceLeft.setScale(2.f, 2.f);
  cursorPieceLeft.setPosition(12 * 2.f, 58 * 2.f);

  cursorPieceRight = cursorPieceLeft;

  letterPos = cursorPosX = cursorPosY = currTable = 0;

  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");

  nameLabel = new sf::Text("", *font);
  nameLabel->setScale(1.f, 1.f);
  
  name = std::string(8, '_'); // 8 characters allowed

  for(int i = 0; i < name.size(); i++) {
    if (i >= folderName.size()) break;

    if (folderName[i] == ' ') continue;

    name[i] = folderName[i];
  }

  animatorLeft = Animation("resources/ui/letter_cursor.animation");
  animatorLeft.Load();
  animatorLeft.SetAnimation("BLINK_L");
  animatorLeft << Animator::Mode::Loop;

  animatorRight = Animation("resources/ui/letter_cursor.animation");
  animatorRight.Load();
  animatorRight.SetAnimation("BLINK_R");
  animatorRight << Animator::Mode::Loop;

  table = {
    { // First column
      {"A", "B", "C", "D", "E"},
      {"F", "G", "H", "I", "J"},
      {"K", "L", "M", "N", "O"},
      {"P", "Q", "R", "S", "T"},
      {"U", "V", "W", "X", "Y"},
      {"Z"}
    },
    { // Second column
      {"a", "b", "c", "d", "e"},
      {"f", "g", "h", "i", "j"},
      {"k", "l", "m", "n", "o"},
      {"p", "q", "r", "s", "t"},
      {"u", "v", "w", "x", "y"},
      {"z"}
    },
    { // Third column are actions
      {"NEXT"},
      {"BACK"},
      {"OK"},
      {"END"}
    }
  };
}

FolderChangeNameScene::~FolderChangeNameScene() {

}

void FolderChangeNameScene::onStart() {
  leave = false;
}

void FolderChangeNameScene::onUpdate(double elapsed) {
  this->elapsed += float(elapsed);

  bool executeAction = false; // Did we execute the action this frame?

  auto lastX = cursorPosX;
  auto lastY = cursorPosY;

  if (!leave) {
    if (INPUT.Has(EventTypes::PRESSED_CANCEL)) {
      if (letterPos != 0 || name[letterPos] != '_') {
        AUDIO.Play(AudioType::CHIP_CANCEL);
      }
      else {
        AUDIO.Play(AudioType::CHIP_ERROR);
      }
      
      if (letterPos + 1 == 9) {
        letterPos--;
      }
      else {
        name[letterPos--] = '_';
        letterPos = std::max(0, letterPos);
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_LEFT)) {
    cursorPosX--;
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT)) {
    cursorPosX++;
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_UP)) {
    cursorPosY--;
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_DOWN)) {
    cursorPosY++;
    }
    else if (INPUT.Has(EventTypes::PRESSED_SCAN_LEFT)) {
      DoBACK();
    }
    else if (INPUT.Has(EventTypes::PRESSED_SCAN_RIGHT)) {
      DoNEXT();
    }
    else if (INPUT.Has(EventTypes::PRESSED_QUICK_OPT)) {
      // hard coded, if columns change this must change too
      cursorPosX = 0;
      cursorPosY = 2;
      currTable = 2;
    }
    else if (INPUT.Has(EventTypes::PRESSED_CONFIRM)) {
      executeAction = true;
    }
  }

  // Check bounds of cursorPosX and cursorPosY, update current table and pos if need to

  // Check X pos
  if (lastX != cursorPosX) {
    AUDIO.Play(AudioType::CHIP_SELECT);

    if (cursorPosX < 0) {
      currTable--;

      if (currTable < 0) {
        currTable = 0;
        cursorPosX = 0;
      }
      else {
        cursorPosY = std::min(cursorPosY, int(table[currTable].size()) - 1);

        cursorPosX = int(table[currTable][cursorPosY].size()) - 1;
      }
    }

    cursorPosY = std::min(cursorPosY, int(table[currTable].size()) - 1);

    if (cursorPosX > table[currTable][cursorPosY].size() - 1) {
      currTable++;

      // base 0, 2 is max of the three tables
      if (currTable > 2) {
        currTable = 2;
        cursorPosY = std::min(cursorPosY, int(table[currTable].size()) - 1);
        cursorPosX = int(table[currTable][cursorPosY].size()) - 1;
      }
      else {
        cursorPosY = std::min(cursorPosY, int(table[currTable].size()) - 1);
        cursorPosX = 0;
      }
    }
  }

  // Check Y pos
  if (lastY != cursorPosY) {
    AUDIO.Play(AudioType::CHIP_SELECT);

    if (cursorPosY < 0) {
      cursorPosY = 0;
    }

    if (cursorPosY > table[currTable].size() - 1) {
      cursorPosY = int(table[currTable].size()) - 1;
    }

    // keep x in bounds too
    cursorPosX = std::min(cursorPosX, int(table[currTable][cursorPosY].size() - 1));
  }

  // Update flashing cursor position
  auto tableOffset = (88.f)*currTable;
  auto startX = 12.f;
  auto startY = 60.f;
  auto offsetx = 16.f * cursorPosX;
  auto offsety = 16.f * cursorPosY;

  cursorPieceLeft.setPosition (2.f*(startX + offsetx + tableOffset), 2.f*(startY + offsety));

  // Last column has wide text e.g. "NEXT", "BACK", ... ensure we fit those words
  if (currTable == 2) { offsetx += 22; if (cursorPosY == 2) { offsetx -= 14; } else if (cursorPosY == 3) { offsetx -= 8; } }

  cursorPieceRight.setPosition(2.f*(startX + offsetx + tableOffset + 4.0f), 2.f*(startY + offsety));

  // Resolve action command if any
  if (executeAction) {
    this->ExecuteAction(currTable, cursorPosX, cursorPosY);
  }

  animatorLeft.Update ((float)elapsed, cursorPieceLeft);
  animatorRight.Update((float)elapsed, cursorPieceRight);

  auto c = name[letterPos];
}

void FolderChangeNameScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(bg);
  ENGINE.Draw(cursorPieceLeft);
  ENGINE.Draw(cursorPieceRight);

  auto blink = int(this->elapsed * 3000) % 1000 < 500;

  for (int i = 0; i < name.size(); i++) {
    if(i == letterPos && blink)
      continue;

    nameLabel->setString(name[i]);
    nameLabel->setPosition((65 + (i*8)) * 2.f, 15 * 2.f);

    ENGINE.Draw(nameLabel);
  }

  // 9th character is a special * asterisk to denote the end of input
  if (letterPos + 1 == 9 && !blink) {
    nameLabel->setString('*');
    nameLabel->setPosition((65 + (8 * 8)) * 2.f, 15 * 2.f);

    ENGINE.Draw(nameLabel);
  }
}
