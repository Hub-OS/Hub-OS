#include <Segues/DiamondTileCircle.h>
#include "bnTitleScene.h"
#include "bnConfigScene.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "overworld/bnOverworldHomepage.h"

using namespace swoosh::types;

void TitleScene::CenterLabel()
{
  // update label position
  auto bounds = startLabel.GetLocalBounds();
  startLabel.setOrigin(bounds.width / 2.0f, bounds.height);
}

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
  std::shared_ptr<sf::Texture> logo;
  
  if (getController().CommandLineValue<std::string>("locale") == "jp") {
    logo = Textures().LoadTextureFromFile("resources/scenes/title/tile.png");
  }
  else {
    logo = Textures().LoadTextureFromFile("resources/scenes/title/tile_en.png");
  }


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

  startLabel.setPosition(sf::Vector2f(240.0f, 240.f));
  startLabel.setScale(2.f, 2.f);

  startLabel.SetString("Loading, Please Wait");
  CenterLabel();

  ConfigSettings config = Input().GetConfigSettings();
  WebServerInfo web = config.GetWebServerInfo();
  WEBCLIENT.ConnectToWebServer(web.version.c_str(), web.URL.c_str(), web.port);
  loginResult = WEBCLIENT.SendLoginCommand(web.user, web.password);

  setView(sf::Vector2u(480, 320));
}

void TitleScene::onStart()
{
  ConfigSettings config = Input().GetConfigSettings();

  // TODO: sfx and music level should already be in 0-100 values imo...
  Audio().SetChannelVolume(((float)config.GetSFXLevel()/3.f)*100.f);
  Audio().SetStreamVolume(((float)config.GetMusicLevel() / 3.f) * 100.f);

  // stream some music while we wait
  Audio().Stream("resources/loops/loop_theme.ogg");

  // Begin performing tasks in the background
  LaunchTasks();
}

void TitleScene::onUpdate(double elapsed)
{
  try {
    // If not ready, do no proceed past this point!
    if (IsComplete() == false || progress < 100) {
      ellipsis = (ellipsis + 1) % 4;
      auto dots = std::string(static_cast<size_t>(ellipsis) + 1, '.');

      if (progress < total) {
        progress++;
      }

      std::string percent = std::to_string(this->progress) + "%";
      std::string label = taskStr + ": " + percent + dots;
      startLabel.SetString(label);

      Poll();

      return;
    } else if (loginResult.valid() && !is_ready(loginResult)) {
      startLabel.SetString("Loggin in...");
      CenterLabel();
      return;
    }
    else {
#if defined(__ANDROID__)
      startLabel.SetString("TAP SCREEN");
#else
      startLabel.SetString("PRESS START");
      CenterLabel();
#endif
    }

    static bool doOnce = true;

    if (doOnce) {
      doOnce = false;

      if (loginResult.valid()) {
        if (loginResult.get()) {
          Logger::Logf("Logged in successfully!");
        }
        else {
          Logger::Logf("Could not log in!");
        }
      }
    }

    if (Input().GetAnyKey() == sf::Keyboard::Enter && !pressedStart) {
      pressedStart = true;

      // We want the next screen to be the main menu screen
      using tx = segue<DiamondTileCircle>::to<Overworld::Homepage>;
      getController().push<tx>();

      /*if (!loginSelected) {
        getController().push<ConfigScene>();
      }*/

      // Zoom out and start a segue effect
      //getController().pop<segue<DiamondTileCircle>>();
    }
  }
  catch (std::future_error& err) {
    Logger::Logf("future_error: %s", err.what());
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
  this->total = unsigned(progress * 100);
  this->taskStr = taskName;
  startLabel.SetString(this->taskStr + " 00%");
  CenterLabel();

  Logger::Logf("[%.2f] Began task %s", progress, taskName.c_str());
}

void TitleScene::onTaskComplete(const std::string & taskName, float progress)
{
  this->total = unsigned(progress * 100);
  this->taskStr = taskName;

  Logger::Logf("[%.2f] Completed task %s", progress, taskName.c_str());
}