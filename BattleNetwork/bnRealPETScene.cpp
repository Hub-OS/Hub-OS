#include <Poco/Net/NetException.h>
#include <Poco/Net/DNS.h>
#include <Poco/Net/HostEntry.h>
#include <Segues/BlackWashFade.h>
#include "bnRealPETScene.h"
#include "overworld/bnOverworldOnlineArea.h"
#include "netplay/bnNetPlayConfig.h"
#include "bnMessage.h"
#include "bnGameSession.h"

#include <Swoosh/ActivityController.h>
#include <Segues/PushIn.h>
#include <Segues/Checkerboard.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/DiamondTileSwipe.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "bnCurrentTime.h"
#include "bnBlockPackageManager.h"
#include "bnMobPackageManager.h"
#include "bnCardFolderCollection.h"
#include "bnGameSession.h"
#include "bnGameUtils.h"

// scenes
#include "bnFolderScene.h"
#include "bnSelectNaviScene.h"
#include "bnSelectMobScene.h"
#include "bnLibraryScene.h"
#include "bnConfigScene.h"
#include "bnFolderScene.h"
#include "bnKeyItemScene.h"
#include "bnMailScene.h"
#include "bnPlayerCustScene.h"
#include "battlescene/bnMobBattleScene.h"
#include "netplay/bnMatchMakingScene.h"


using namespace swoosh::types;

constexpr size_t DEFAULT_PORT = 8765;

/// \brief Thunk to populate menu options to callbacks
namespace {
  auto MakeOptions = [](RealPET::Homepage* scene) -> Overworld::PersonalMenu::OptionsList {
    return {
      { "chip_folder", std::bind(&RealPET::Homepage::GotoChipFolder, scene) },
      { "navi",        std::bind(&RealPET::Homepage::GotoNaviSelect, scene) },
      { "mail",        std::bind(&RealPET::Homepage::GotoMail, scene) },
      { "key_items",   std::bind(&RealPET::Homepage::GotoKeyItems, scene) },
      { "mob_select",  std::bind(&RealPET::Homepage::GotoMobSelect, scene) },
      { "config",      std::bind(&RealPET::Homepage::GotoConfig, scene) },
      { "sync",        std::bind(&RealPET::Homepage::GotoPVP, scene) }
    };
  };
}

RealPET::Homepage::Homepage(swoosh::ActivityController& controller) :
  lastIsConnectedState(false),
  playerSession(std::make_shared<PlayerSession>()),
  Scene(controller)
{
  auto& session = getController().Session();
  bool loaded = session.LoadSession("profile.bin");

  // folders may be blank if session was unable to load a collection
  folders = &session.GetCardFolderCollection();

  if (loaded) {
    NaviEquipSelectedFolder();
  }

  setView(sf::Vector2u(480, 320));

  std::string destination_ip = getController().Session().GetKeyValue("homepage_warp:0");

  int remotePort = getController().CommandLineValue<int>("remotePort");
  host = getController().CommandLineValue<std::string>("cyberworld");

  if (host.empty()) {
    size_t colon = destination_ip.find(':', 0);

    if (colon > 0 && colon != std::string::npos) {
      host = destination_ip.substr(0, colon);
      remotePort = std::atoi(destination_ip.substr(colon + 1u).c_str());
    }
    else {
      host = destination_ip;
    }
  }

  if (remotePort > 0 && host.size()) {
    try {
      remoteAddress = Poco::Net::SocketAddress(host, remotePort);

      packetProcessor = std::make_shared<Overworld::PollingPacketProcessor>(
        remoteAddress,
        Net().GetMaxPayloadSize(),
        [this](ServerStatus status, size_t maxPayloadSize) { UpdateServerStatus(status, maxPayloadSize); }
      );

      Net().AddHandler(remoteAddress, packetProcessor);
    }
    catch (Poco::IOException&) {}
  }
}

RealPET::Homepage::~Homepage() {
}

void RealPET::Homepage::UpdateServerStatus(ServerStatus status, uint16_t serverMaxPayloadSize) {
  serverStatus = status;
  maxPayloadSize = serverMaxPayloadSize;

  // EnableNetWarps(status == ServerStatus::online);
}

void RealPET::Homepage::onUpdate(double elapsed)
{
  if (IsInFocus()) {
    // HandleInput();
  }

  menuSystem.Update(elapsed);
}

void RealPET::Homepage::onDraw(sf::RenderTexture& surface)
{
  surface.draw(menuSystem);
}

void RealPET::Homepage::onStart()
{
  Audio().Stream("resources/loops/loop_overworld.ogg", true);

#ifdef __ANDROID__
    StartupTouchControls();
#endif

  // TODO: this is all just test stuff 
  Inbox& inbox = playerSession->inbox;

  std::shared_ptr<Background> bg = std::make_shared<LanBackground>();
  PA* pa = &GetProgramAdvance();
  Game* controller = &getController();
  Inbox::OnMailReadCallback virusTrigger;
  virusTrigger.Slot([bg, pa, controller, this](Inbox::Mail& mail) {
    std::string navi = this->GetCurrentNaviID();
    mail.body = "This mail had a virus. It's clean now.";

    {
      PlayerPackageManager& playerPackages = controller->PlayerPackagePartitioner().GetPartition(Game::LocalPartition);
      MobPackageManager& mobPackages = controller->MobPackagePartitioner().GetPartition(Game::LocalPartition);

      if (mobPackages.Size() == 0 || playerPackages.Size() == 0) return;

      PlayerMeta& playerMeta = playerPackages.FindPackageByID(navi);
      MobMeta& mobMeta = mobPackages.FindPackageByID(mobPackages.FirstValidPackage());

      GameUtils(*controller).LaunchMobBattle(playerMeta, mobMeta, bg, *pa, nullptr);
    }
    });

  sf::Texture mugshot = *Textures().LoadFromFile("resources/ow/prog/prog_mug.png");
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::announcement, "Welcome", "NO-TITLE", "This is your first email!", mugshot });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::dm, "HELLO", "KERISTERO", "try gravy" });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::dm_w_attachment, "ELLO", "DESTROYED", "ello govna" });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::important, "FIRE", "NO-TITLE", "There's a fire in the undernet!", mugshot });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::mission, "MISSING", "ANON", "Can you find my missing data? It would really help me out right now... Or don't if it's too hard, I understand..." });
  inbox.AddMail(Inbox::Mail{ Inbox::Icons::dm, "Test", "NO-TITLE", "Just another test-\n\n\nWait! This email contained a virus!", mugshot, virusTrigger });
}

void RealPET::Homepage::onResume()
{
  Audio().Stream("resources/loops/loop_overworld.ogg", true);

  if (packetProcessor) {
    Net().AddHandler(remoteAddress, packetProcessor);
  }

  getController().Session().SaveSession("profile.bin");
}

void RealPET::Homepage::onLeave()
{
  if (packetProcessor) {
    Net().DropProcessor(packetProcessor);
  }
}

void RealPET::Homepage::onExit()
{
}

void RealPET::Homepage::onEnter()
{
  RefreshNaviSprite();

}

void RealPET::Homepage::onEnd()
{
  if (packetProcessor) {
    Net().DropProcessor(packetProcessor);
  }
}

void RealPET::Homepage::RefreshNaviSprite()
{
  // Only refresh all data and graphics if this is a new navi
  if (lastSelectedNaviId == currentNaviId && !lastSelectedNaviId.empty()) return;

  PlayerPackageManager& packageManager = getController().PlayerPackagePartitioner().GetPartition(Game::LocalPartition);
  if (!packageManager.HasPackage(currentNaviId)) {
    currentNaviId = packageManager.FirstValidPackage();
    getController().Session().SetKeyValue("SelectedNavi", currentNaviId);
  }

  lastSelectedNaviId = currentNaviId;

  auto& meta = packageManager.FindPackageByID(currentNaviId);

  // refresh menu widget too
  playerSession->health = meta.GetHP();
  playerSession->maxHealth = meta.GetHP();
}

void RealPET::Homepage::NaviEquipSelectedFolder()
{
  GameSession& session = getController().Session();
  std::string naviId = session.GetKeyValue("SelectedNavi");
  if (!naviId.empty()) {
    currentNaviId = naviId;
    RefreshNaviSprite();

    std::string folderStr = session.GetKeyValue("FolderFor:" + naviId);
    if (!folderStr.empty()) {
      // preserve our selected folder
      if (int index = folders->FindFolder(folderStr); index >= 0) {
        folders->SwapOrder(index, 0); // Select this folder again
      }
    }
  }
  else {
    currentNaviId = getController().PlayerPackagePartitioner().GetPartition(Game::LocalPartition).FirstValidPackage();
    session.SetKeyValue("SelectedNavi", currentNaviId);
  }
}

void RealPET::Homepage::GotoChipFolder()
{
  Audio().Play(AudioType::CHIP_DESC);

  if (!folders) {
    Logger::Log(LogLevel::debug, "folder collection was nullptr");
    return;
  }

  using effect = segue<PushIn<direction::left>, milliseconds<500>>;
  getController().push<effect::to<FolderScene>>(*folders);
}

void RealPET::Homepage::GotoNaviSelect()
{
  // Navi select
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<Checkerboard, milliseconds<250>>;
  getController().push<effect::to<SelectNaviScene>>(currentNaviId);
}

void RealPET::Homepage::GotoConfig()
{
  // Config Select on PC
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<DiamondTileSwipe<direction::right>, milliseconds<500>>;
  getController().push<effect::to<ConfigScene>>();
}

void RealPET::Homepage::GotoMobSelect()
{
  MobPackageManager& pm = getController().MobPackagePartitioner().GetPartition(Game::LocalPartition);
  if (pm.Size() == 0) {
    menuSystem.EnqueueMessage("No enemy mods installed.");
    return;
  }

  std::unique_ptr<CardFolder> folder;
  CardFolder* f = nullptr;

  if (folders->GetFolder(0, f)) {
    folder = f->Clone();
  }
  else {
    folder = std::make_unique<CardFolder>();
  }

  SelectMobScene::Properties props{ currentNaviId, std::move(folder), programAdvance, std::make_shared<LanBackground>() };
  using effect = segue<PixelateBlackWashFade, milliseconds<500>>;
  Audio().Play(AudioType::CHIP_DESC);
  getController().push<effect::to<SelectMobScene>>(std::move(props));
}

void RealPET::Homepage::GotoPVP()
{
  std::unique_ptr<CardFolder> folder;

  CardFolder* f = nullptr;

  if (folders->GetFolder(0, f)) {
    folder = f->Clone();
  }
  else {
    folder = std::make_unique<CardFolder>();
  }

  Audio().Play(AudioType::CHIP_DESC);
  using effect = segue<PushIn<direction::down>, milliseconds<500>>;
  getController().push<effect::to<MatchMakingScene>>(currentNaviId, std::move(folder), programAdvance);
}

void RealPET::Homepage::GotoMail()
{
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<BlackWashFade, milliseconds<500>>;
  getController().push<effect::to<MailScene>>(playerSession->inbox);
}

void RealPET::Homepage::GotoKeyItems()
{
  // Config Select on PC
  Audio().Play(AudioType::CHIP_DESC);

  using effect = segue<BlackWashFade, milliseconds<500>>;

  getController().push<effect::to<KeyItemScene>>(items);
}

std::string& RealPET::Homepage::GetCurrentNaviID()
{
  return currentNaviId;
}

PA& RealPET::Homepage::GetProgramAdvance() {
  return programAdvance;
}

std::optional<CardFolder*> RealPET::Homepage::GetSelectedFolder() {
  CardFolder* folder;

  if (folders->GetFolder(0, folder)) {
    return folder;
  }
  else {
    return {};
  }
}

bool RealPET::Homepage::IsInFocus()
{
  return getController().getCurrentActivity() == this;
}
