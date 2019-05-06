#include <time.h>
#include "bnFolderScene.h"
#include "bnChipLibrary.h"
#include "bnChipFolder.h"
#include "bnMemory.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnShaderResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnTimer.h"
#include "bnEngine.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

int FolderScene::Run()
{
  Camera camera(ENGINE.GetDefaultView());

  ENGINE.SetCamera(camera);

  // Menu name font
  sf::Font* font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  sf::Text* menuLabel = new sf::Text("Library", *font);
  menuLabel->setCharacterSize(15);
  menuLabel->setPosition(sf::Vector2f(20.f, 5.0f));

  // Selection input delays
  double maxSelectInputCooldown = 0.5; // half of a second
  double selectInputCooldown = maxSelectInputCooldown;

  // Chip UI font
  sf::Font *chipFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  sf::Text *chipLabel = new sf::Text("", *chipFont);
  chipLabel->setPosition(275.f, 15.f);

  sf::Font *numberFont = TEXTURES.LoadFontFromFile("resources/fonts/mgm_nbr_pheelbert.ttf");
  sf::Text *numberLabel = new sf::Text("", *numberFont);
  numberLabel->setOutlineColor(sf::Color(48, 56, 80));
  numberLabel->setOutlineThickness(2.f);
  numberLabel->setScale(0.8f, 0.8f);
  numberLabel->setOrigin(numberLabel->getLocalBounds().width, 0);
  numberLabel->setPosition(sf::Vector2f(170.f, 28.0f));

  // Chip description font
  sf::Font *chipDescFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthin_regular.ttf");
  sf::Text* chipDesc = new sf::Text("", *chipDescFont);
  chipDesc->setCharacterSize(30);
  chipDesc->setPosition(sf::Vector2f(20.f, 185.0f));
  //chipDesc->setLineSpacing(5);
  chipDesc->setFillColor(sf::Color::Black);

  // folder menu graphic
  sf::Sprite bg(*TEXTURES.GetTexture(TextureType::FOLDER_VIEW_BG));
  bg.setScale(2.f, 2.f);

  sf::Sprite folderDock(*TEXTURES.GetTexture(TextureType::FOLDER_DOCK));
  folderDock.setScale(2.f, 2.f);
  folderDock.setPosition(2.f, 30.f);

  sf::Sprite scrollbar(*TEXTURES.GetTexture(TextureType::FOLDER_SCROLLBAR));
  scrollbar.setScale(2.f, 2.f);
  scrollbar.setPosition(410.f, 60.f);

  sf::Sprite stars(*TEXTURES.GetTexture(TextureType::FOLDER_RARITY));
  stars.setScale(2.f, 2.f);

  sf::Sprite chipHolder(*TEXTURES.GetTexture(TextureType::FOLDER_CHIP_HOLDER));
  chipHolder.setScale(2.f, 2.f);
  chipHolder.setPosition(4.f, 35.f);

  sf::Sprite element(*TEXTURES.GetTexture(TextureType::ELEMENT_ICON));
  element.setScale(2.f, 2.f);
  element.setPosition(2.f*25.f, 146.f);

  // Current chip graphic
  sf::Sprite chip(*TEXTURES.GetTexture(TextureType::CHIP_CARDS));
  sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID(0);
  chip.setTextureRect(cardSubFrame);
  chip.setScale(2.f, 2.f);
  chip.setPosition(83.f, 93.f);
  chip.setOrigin(chip.getLocalBounds().width/2.0f, chip.getLocalBounds().height/2.0f);

  sf::Sprite chipIcon(*TEXTURES.GetTexture(TextureType::CHIP_ICONS));
  chipIcon.setScale(2.f, 2.f);

  Timer chipRevealTimer;
  chipRevealTimer.Start();

  // Transition
  sf::Shader& transition = *SHADERS.GetShader(ShaderType::TRANSITION);
  transition.setUniform("texture", sf::Shader::CurrentTexture);
  transition.setUniform("map", *TEXTURES.GetTexture(TextureType::NOISE_TEXTURE));
  transition.setUniform("progress", 0.f);
  float transitionProgress = 0.9f;
  ENGINE.RevokeShader();

  int maxChipsOnScreen = 7;
  int currChipIndex = 0;
  int numOfChips = CHIPLIB.GetSize();

  bool gotoNextScene = false;

  Clock clock;
  float elapsed = 0.0f;
  float totalTime = 0.f;
  while (ENGINE.Running()) {

    float elapsedSeconds = clock.restart().asSeconds();
    totalTime += elapsedSeconds;

    float FPS = 0.f;

    if (elapsedSeconds > 0.f) {
      FPS = 1.0f / elapsedSeconds;
      std::string fpsStr = std::to_string(FPS);
      fpsStr.resize(4);
      ENGINE.GetWindow()->setTitle(sf::String(std::string("FPS: ") + fpsStr));
    }

    INPUT.update();

    ENGINE.Clear();

    camera.Update(elapsed);

    ENGINE.Draw(bg);
    ENGINE.Draw(menuLabel);

    ENGINE.DrawUnderlay();
    ENGINE.DrawLayers();
    ENGINE.DrawOverlay();

    ENGINE.Draw(folderDock);
    ENGINE.Draw(chipHolder);

    // ScrollBar limits: Top to bottom screen position when selecting first and last chip respectively
    float top = 50.0f; float bottom = 230.0f;
    float depth = ((float)currChipIndex / (float)numOfChips)*bottom;
    scrollbar.setPosition(452.f, top + depth);

    ENGINE.Draw(scrollbar);
  
    // Move the chip library iterator to the current highlighted chip
    ChipLibrary::Iter iter = CHIPLIB.Begin();

    for (int j = 0; j < currChipIndex; j++) {
      iter++;
    }

    sf::IntRect cardSubFrame = TEXTURES.GetCardRectFromID(iter->GetID());
    chip.setTextureRect(cardSubFrame);
    chip.setScale((float)Ease::Linear(chipRevealTimer.GetElapsed() / 1000.0, 0.25)*2.0f, 2.0f);
    ENGINE.Draw(chip, false);

    // This draws the currently highlighted chip
    if (iter->GetDamage() > 0) {
      chipLabel->setFillColor(sf::Color::White);
      chipLabel->setString(std::to_string(iter->GetDamage()));
      chipLabel->setOrigin(chipLabel->getLocalBounds().width+chipLabel->getLocalBounds().left, 0);
      chipLabel->setPosition(2.f*(70.f), 135.f);

      ENGINE.Draw(chipLabel, false);
    }

    chipLabel->setOrigin(0, 0);
    chipLabel->setFillColor(sf::Color::Yellow);
    chipLabel->setPosition(2.f*14.f, 135.f);
    chipLabel->setString(std::string() + iter->GetCode());
    ENGINE.Draw(chipLabel, false);

    std::string formatted = FormatChipDesc(iter->GetDescription());
    chipDesc->setString(formatted);
    ENGINE.Draw(chipDesc, false);

    int offset = (int)(iter->GetElement());
    element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
    element.setPosition(2.f*25.f, 142.f);
    ENGINE.Draw(element, false);

    // Now that we are at the viewing range, draw each chip in the list
    for (int i = 0; i < maxChipsOnScreen; i++) {

      if (currChipIndex + i < numOfChips) {
        chipIcon.setTextureRect(TEXTURES.GetIconRectFromID(iter->GetIconID()));
        chipIcon.setPosition(2.f*104.f, 65.0f + (32.f*i));
        ENGINE.Draw(chipIcon, false);

        chipLabel->setFillColor(sf::Color::White);
        chipLabel->setPosition(2.f*120.f, 60.0f + (32.f*i));
        chipLabel->setString(iter->GetShortName());
        ENGINE.Draw(chipLabel, false);


        int offset = (int)(iter->GetElement());
        element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
        element.setPosition(2.f*173.f, 65.0f + (32.f*i));
        ENGINE.Draw(element, false);

        chipLabel->setOrigin(0, 0);
        chipLabel->setPosition(2.f*190.f, 60.0f + (32.f*i));
        chipLabel->setString(std::string() + iter->GetCode());
        ENGINE.Draw(chipLabel, false);

        //Draw rating
        unsigned rarity = iter->GetRarity()-1;
        stars.setTextureRect(sf::IntRect(0, 15*rarity, 22, 14));
        stars.setPosition(2.f*199.f, 74.0f + (32.f*i));
        ENGINE.Draw(stars, false);

        iter++;
      }
    }

    // Scene keyboard controls
    if (!gotoNextScene && transitionProgress == 0.f) {
      if (INPUT.has(PRESSED_UP)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          // Go to previous mob 
          selectInputCooldown = maxSelectInputCooldown;
          currChipIndex--;
          AUDIO.Play(AudioType::CHIP_SELECT);

          chipRevealTimer.Reset();
        }
      }
      else if (INPUT.has(PRESSED_DOWN)) {
        selectInputCooldown -= elapsed;

        if (selectInputCooldown <= 0) {
          // Go to next mob 
          selectInputCooldown = maxSelectInputCooldown;
          currChipIndex++;
          AUDIO.Play(AudioType::CHIP_SELECT);

          chipRevealTimer.Reset();
        }
      }
      else {
        selectInputCooldown = 0;
      }

      currChipIndex = std::max(0, currChipIndex);
      currChipIndex = std::min(numOfChips-1, currChipIndex);

      if (INPUT.has(PRESSED_B)) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      }
    }

    if (elapsed > 0) {
      if (gotoNextScene) {
        transitionProgress += 1 * elapsed;
      }
      else {
        transitionProgress -= 1 * elapsed;
      }
    }

    transitionProgress = std::max(0.f, transitionProgress);
    transitionProgress = std::min(1.f, transitionProgress);

    if (transitionProgress >= 1.f) {
      return 2;

      gotoNextScene = false;
    }

    sf::Texture postprocessing = ENGINE.GetPostProcessingBuffer().getTexture(); // Make a copy
    postprocessing.flip(true);
    sf::Sprite transitionPost;
    transitionPost.setTexture(postprocessing);

    transition.setUniform("progress", transitionProgress);

    LayeredDrawable* bake = new LayeredDrawable(transitionPost);
    bake->SetShader(&transition);

    ENGINE.Draw(bake);
    delete bake;

    // Write contents to screen (always last step)
    ENGINE.Display();

    elapsed = static_cast<float>(clock.getElapsedTime().asSeconds());
  }
  delete font;
  delete chipFont;
  delete chipDescFont;
  delete numberFont;
  delete menuLabel;
  delete numberLabel;
  delete chipDesc;

  ENGINE.RevokeShader();

  return 1; // signal OK to the last scene
}

std::string FolderScene::FormatChipDesc(const std::string && desc)
{
  // Chip docks can only fit 8-9 characters per line, 3 lines total
  std::string output = desc;

  int index = 0;
  int perLine = 0;
  int line  = 0;
  int wordIndex = -1; // If we are breaking on a word
  while (index != desc.size()) {
    if (desc[index] != ' ' && wordIndex == -1) {
      wordIndex = index;
    }
    else if (desc[index] == ' ' && wordIndex > -1) {
      wordIndex = -1;
    }

    if (perLine > 0 && perLine % 9 == 0) {
      if (wordIndex > -1) {
        // Line break at the last word
        while (desc[index] == ' ') { index++; }
        output.insert(wordIndex, "\n");
        index = wordIndex; // Start counting from here
        while (desc[index] == ' ') { index++; }
      }
      else {
        // Line break at the next word
        while (desc[index] == ' ') { index++; }
        output.insert(index, "\n");
        while (desc[index] == ' ') { index++; }
      }
      line++;
      perLine = 0;
      wordIndex = -1;
    }

    //if (line == 3) {
    //  break;
   // }

    if (desc[index] != ' ') {
      perLine++;
    }

    index++;
  }

  return output;
}
