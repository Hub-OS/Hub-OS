#include <Segues/DiamondTileCircle.h>
#include "bnTitleScene.h"
#include "bnConfigScene.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "overworld/bnOverworldHomepage.h"

using namespace swoosh::types;

TitleScene::TitleScene(swoosh::ActivityController& controller, TaskGroup&& tasks) : 
  startFont(Font::Style::thick),
  font(Font::Style::small),
  logLabel(font),
  startLabel(startFont),
  inLoadState(true),
  ready(false),
  loadMobs(false),
  LoaderScene(controller, std::move(tasks))
{
  // Title screen logo based on region
#if ONB_REGION_JAPAN
  std::shared_ptr<sf::Texture> logo = Textures().LoadTextureFromFile("resources/scenes/title/tile.png");
#else
  std::shared_ptr<sf::Texture> logo = Textures().LoadTextureFromFile("resources/scenes/title/tile_en.png");
#endif

  bgSprite.setTexture(Textures().LoadTextureFromFile("resources/scenes/title/bg_blue.png"));
  bgSprite.setScale(2.f, 2.f);

  logoSprite.setTexture(logo);
  logoSprite.setOrigin(logoSprite.getLocalBounds().width / 2, logoSprite.getLocalBounds().height / 2);
  sf::Vector2f logoPos = sf::Vector2f(240.f, 160.f);
  logoSprite.setPosition(logoPos);

  // Log output text
  font = Font(Font::Style::small);
  logLabel.setOrigin(0.f, logLabel.GetLocalBounds().height);
  logLabel.setScale(2.f, 2.f);

  // Press Start text
  startFont = Font(Font::Style::thick);

  startLabel.setOrigin(0.f, startLabel.GetLocalBounds().height);
  startLabel.setPosition(sf::Vector2f(180.0f, 240.f));
  startLabel.setScale(2.f, 2.f);

  // When progress is equal to the totalObject count, we are 100% ready
  totalObjects = static_cast<unsigned>(TextureType::TEXTURE_TYPE_SIZE);
  totalObjects += static_cast<unsigned>(AudioType::AUDIO_TYPE_SIZE);
  totalObjects += static_cast<unsigned>(ShaderType::SHADER_TYPE_SIZE);

  setView(sf::Vector2u(480, 320));
}

void TitleScene::onStart()
{
  // stream some music while we wait
  Audio().Stream("resources/loops/loop_theme.ogg");

  // Begin performing tasks in the background
  LaunchTasks();
}

void TitleScene::onUpdate(double elapsed)
{
  // update label position
  startLabel.setOrigin(0.f, startLabel.GetLocalBounds().height);
  startLabel.setPosition(sf::Vector2f(180.0f, 240.f));
  startLabel.setScale(2.f, 2.f);

  // If not ready, do no proceed past this point!
  if (IsComplete() == false) return;

  static bool doOnce = true;

  if (doOnce) {
    doOnce = false;

#if defined(__ANDROID__)
    startLabel.SetString("TAP SCREEN");
#else
    startLabel.SetString("PRESS START");
#endif
  }

  if (Input().GetAnyKey() == sf::Keyboard::Enter && !pressedStart) {
    pressedStart = true;

    // We want the next screen to be the main menu screen
    getController().push<Overworld::Homepage>(loginSelected);

    /*if (!loginSelected) {
      getController().push<ConfigScene>();
    }*/

    // Zoom out and start a segue effect
    //getController().pop<segue<DiamondTileCircle>>();
  }
}

void TitleScene::onLeave()
{
}

void TitleScene::onExit()
{
}

void TitleScene::onEnter()
{
}

void TitleScene::onResume()
{
  // stream the theme song again
  Audio().Stream("resources/loops/loop_theme.ogg");
}

void TitleScene::onDraw(sf::RenderTexture & surface)
{
  surface.draw(bgSprite);
  surface.draw(logoSprite);
  surface.draw(startLabel);
}

void TitleScene::onEnd()
{
}

void TitleScene::onTaskBegin(const std::string & taskName, float progress)
{
  startLabel.SetString("Working..." + taskName);

  Logger::Logf("[%.2f] Began task %s", progress, taskName.c_str());
}

void TitleScene::onTaskComplete(const std::string & taskName, float progress)
{
  startLabel.SetString("Completed..." + taskName);

  Logger::Logf("[%.2f] Completed task %s", progress, taskName.c_str());
}