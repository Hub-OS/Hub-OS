#include "bnTitleScene.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

TitleScene::TitleScene(swoosh::ActivityController * controller, TaskGroup&& tasks)
  : 
  startFont(Font::Style::thick),
  font(Font::Style::small),
  logLabel(font),
  startLabel(startFont),
  selected(0),
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
    totalObjects = (unsigned)TextureType::TEXTURE_TYPE_SIZE;
    totalObjects += (unsigned)AudioType::AUDIO_TYPE_SIZE;
    totalObjects += (unsigned)ShaderType::SHADER_TYPE_SIZE;
}

void TitleScene::onStart()
{
  // stream some music while we wait
  Audio()().Stream("resources/loops/loop_theme.ogg");

  // Begin performing tasks in the background
  LaunchTasks();
}

void TitleScene::onUpdate(double elapsed)
{
  static bool doOnce = true;

  if (IsComplete() && doOnce) {
    doOnce = false;

#if defined(__ANDROID__)
    startLabel.SetString("TAP STRING");
#else
    startLabel.SetString("PRESS START");
#endif
  }

  // update label position
  startLabel.setOrigin(0.f, startLabel.GetLocalBounds().height);
  startLabel.setPosition(sf::Vector2f(180.0f, 240.f));
  startLabel.setScale(2.f, 2.f);
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
  Audio()().Stream("resources/loops/loop_theme.ogg");
}

void TitleScene::onDraw(sf::RenderTexture & surface)
{
}

void TitleScene::onEnd()
{
}

void TitleScene::onTaskBegin(const std::string & taskName, float progress)
{
  startLabel.SetString("Working..." + taskName);

  Logger::Logf("[%.2f] Began task %s", taskName.c_str(), progress);
}

void TitleScene::onTaskComplete(const std::string & taskName, float progress)
{
  Logger::Logf("[%.2f] Completed task %s", taskName.c_str(), progress);
}