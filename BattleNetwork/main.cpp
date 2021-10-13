#include "bnGame.h"
#include "battlescene/bnMobBattleScene.h"
#include "bnPlayerPackageManager.h"
#include "bnMobPackageManager.h"
#include "bnGameOverScene.h"
#include "bnTitleScene.h"
#include "bnACDCBackground.h"
#include "bnMob.h"
#include "bnPlayer.h"
#include "bnEmotions.h"
#include "bnCardFolder.h"
#include "cxxopts/cxxopts.hpp"
#include "netplay/bnNetPlayConfig.h"

int LaunchGame(Game& g, const cxxopts::ParseResult& results);
int HandleBattleOnly(Game& g, TaskGroup tasks, const std::string& playerpath, const std::string& mobpath, bool isURL);

int main(int argc, char** argv) {
  DrawWindow win;
  win.Initialize(DrawWindow::WindowMode::window);
  Game game{ win };

  if (game.GetEndianness() == Endianness::big) {
    Logger::Log("System arch is Big Endian");
  }
  else {
    Logger::Log("System arch is Little Endian");
  }

  cxxopts::Options options("ONB", "Open Net Battle Engine");
  options.add_options()
    ("d,debug", "Enable debugging")
    ("s,singlethreaded", "run logic and draw routines in a single, main thread")
    ("b,battleonly", "Jump into a battle from a package")
    ("mob", "path to mob file on disk", cxxopts::value<std::string>())
    ("moburl", "path to mob file to download from a web address", cxxopts::value<std::string>()->default_value(""))
    ("player", "path to player package on disk", cxxopts::value<std::string>()->default_value(""))
    ("l,locale", "set flair and language to desired target", cxxopts::value<std::string>()->default_value("en"))
    ("p,port", "port for PVP", cxxopts::value<int>()->default_value(std::to_string(NetPlayConfig::OBN_PORT)))
    ("r,remotePort", "remote port for PVP", cxxopts::value<int>()->default_value(std::to_string(NetPlayConfig::OBN_PORT)))
    ("w,cyberworld", "ip address of main hub", cxxopts::value<std::string>()->default_value("0.0.0.0"))
    ("m,mtu", "Maximum Transmission Unit - adjust to send big packets", cxxopts::value<uint16_t>()->default_value(std::to_string(NetManager::DEFAULT_MAX_PAYLOAD_SIZE)));

  try {
    // Go the the title screen to kick off the rest of the app
    if (LaunchGame(game, options.parse(argc, argv)) == EXIT_SUCCESS) {
      // blocking
      game.Run();
    }
  }
  catch (std::exception& e) {
    Logger::Log(e.what());
  }
  catch (...) {
    Logger::Log("Game encountered an unknown exception. Aborting.");
  }

  // finished
  return EXIT_SUCCESS;
}

int LaunchGame(Game& g, const cxxopts::ParseResult& results) {
  g.SetCommandLineValues(results);

  if (g.CommandLineValue<bool>("battleonly")) {
    std::string playerpath = g.CommandLineValue<std::string>("player");
    std::string mobpath = g.CommandLineValue<std::string>("mob");
    std::string moburl = g.CommandLineValue<std::string>("moburl");
    bool url = false;

    if (playerpath.empty()) {
      Logger::Logf("Battleonly mode needs `player` input argument");
      return EXIT_FAILURE;
    }

    if (mobpath.empty() && moburl.empty()) {
      Logger::Logf("Battleonly mode needs `mob` or `moburl` input argument");
      return EXIT_FAILURE;
    }

    if (moburl.size()) {
      mobpath = moburl;
      url = true;
    }

    return HandleBattleOnly(g, g.Boot(results), playerpath, mobpath, url);
  }

  // If single player game, the last screen the player will ever see 
  // is the game over screen so it goes to the bottom of the stack
  // before the TitleSceene:
  // g.push<GameOverScene>(); // <-- uncomment

  g.push<TitleScene>(g.Boot(results));
  return EXIT_SUCCESS;
}

int HandleBattleOnly(Game& g, TaskGroup tasks, const std::string& playerpath, const std::string& mobpath, bool isURL) {
  if (isURL) {
    // TODO: 1. download zip file from URL at `modpath`
    //       2. boot game to load resources
    //       3. install mob package
    //       4. go to battle scene
    Logger::Logf("url unimplemented");
    return EXIT_FAILURE;
  }

  // wait for resources to be available for us
  const unsigned int maxtasks = tasks.GetTotalTasks();
  while (tasks.HasMore()) {
    const std::string taskname = tasks.GetTaskName();
    const unsigned int tasknumber = tasks.GetTaskNumber();
    Logger::Logf("Running %s, [%i/%i]", taskname.c_str(), tasknumber+1u, maxtasks);
    tasks.DoNextTask();
  }

  ResourceHandle handle;

  // Play the pre battle rumble sound
  handle.Audio().Play(AudioType::PRE_BATTLE, AudioPriority::high);

  // Stop music and go to battle screen 
  handle.Audio().StopStream();

  Field* field = new Field(6, 3);

  // Get the navi we selected
  auto& playermeta = g.PlayerPackageManager().FindPackageByID(playerpath); // TODO: check if path is already installed...
  const std::string& image = playermeta.GetMugshotTexturePath();
  Animation mugshotAnim = Animation() << playermeta.GetMugshotAnimationPath();
  const std::string& emotionsTexture = playermeta.GetEmotionsTexturePath();
  auto mugshot = handle.Textures().LoadTextureFromFile(image);
  auto emotions = handle.Textures().LoadTextureFromFile(emotionsTexture);
  Player* player = playermeta.GetData();

  auto& mobmeta = g.MobPackageManager().FindPackageByID(mobpath); // TODO: check if path is already installed...
  Mob* mob = mobmeta.GetData()->Build(field);

  // Shuffle our new folder
  CardFolder* folder = new CardFolder(); // TODO: Load from file?
  folder->Shuffle();

  // Queue screen transition to Battle Scene with a white fade effect
  // just like the game
  if (!mob->GetBackground()) {
    mob->SetBackground(std::make_shared<ACDCBackground>());
  }

  PA programAdvance;

  MobBattleProperties props{
    { *player, programAdvance, folder, field, mob->GetBackground() },
    MobBattleProperties::RewardBehavior::take,
    { mob },
    sf::Sprite(*mugshot),
    mugshotAnim,
    emotions,
  };

  g.push<MobBattleScene>(props);
  return EXIT_SUCCESS;
}