#include <Poco/Net/NetException.h>
#include <Poco/Net/DNS.h>
#include <Poco/Net/HostEntry.h>
#include <Swoosh/ActivityController.h>
#include <Segues/BlackWashFade.h>
#include <Segues/PushIn.h>
#include <Segues/Checkerboard.h>
#include <Segues/PixelateBlackWashFade.h>
#include <Segues/DiamondTileSwipe.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "bnHomepageScene.h"
#include "../overworld/bnOverworldOnlineArea.h"
#include "../netplay/bnNetPlayConfig.h"
#include "../bnMessage.h"
#include "../bnGameSession.h"
#include "../bnCurrentTime.h"
#include "../bnBlockPackageManager.h"
#include "../bnMobPackageManager.h"
#include "../bnCardFolderCollection.h"
#include "../bnGameSession.h"
#include "../bnGameUtils.h"
#include "../bnFolderScene.h"
#include "../bnSelectNaviScene.h"
#include "../bnSelectMobScene.h"
#include "../bnLibraryScene.h"
#include "../bnConfigScene.h"
#include "../bnFolderScene.h"
#include "../bnKeyItemScene.h"
#include "../bnMailScene.h"
#include "../bnPlayerCustScene.h"
#include "../battlescene/bnMobBattleScene.h"
#include "../netplay/bnMatchMakingScene.h"

using namespace swoosh::types;

constexpr size_t DEFAULT_PORT = 8765;

/// \brief Thunk to populate menu options to callbacks
namespace {
  auto MakeOptions = [](RealPET::Homepage* scene) -> Menu::OptionsList {
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
  menuWidget(playerSession, MakeOptions(this)),
  Scene(controller)
{
  auto& session = getController().Session();
  bool loaded = session.LoadSession(FilePaths::PROFILE);

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

  // Load audio and graphics resources ....

  bgTexture = Textures().LoadFromFile(TexturePaths::REAL_PET_BG);
  bgSprite.setTexture(*bgTexture);
  bgSprite.setScale(2.f, 2.f);

  folderTexture = Textures().LoadFromFile(TexturePaths::PET_PARTICLE_FOLDERS);
  folderAnim = Animation(AnimationPaths::PET_PARTICLE_FOLDERS);

  windowTexture = Textures().LoadFromFile(TexturePaths::PET_PARTICLE_WINDOWS);
  windowAnim = Animation(AnimationPaths::PET_PARTICLE_WINDOWS) << "default";

  jackinTexture = Textures().LoadFromFile(TexturePaths::PET_JACKIN);
  jackinAnim = Animation(AnimationPaths::PET_JACKIN) << "default";
  jackinSprite.setTexture(*jackinTexture);
  jackinSprite.setPosition(-10.0f * 2.f, 0.f);
  jackinSprite.setScale(2.f, 2.f);
  jackinAnim.Refresh(jackinSprite);

  jackinsfx = Audio().LoadFromFile(SoundPaths::PET_JACKIN);

  menuTexture = Textures().LoadFromFile(TexturePaths::PET_MENU);
  menuAnim = Animation(AnimationPaths::PET_MENU);

  miscMenuTexture = Textures().LoadFromFile(TexturePaths::PET_MISC_MENU);
  miscMenuAnim = Animation(AnimationPaths::PET_MISC_MENU);

  InitializeFolderParticles();
  InitializeWindowParticles();
}

RealPET::Homepage::~Homepage() {
}

void RealPET::Homepage::UpdateServerStatus(ServerStatus status, uint16_t serverMaxPayloadSize) {
  serverStatus = status;
  maxPayloadSize = serverMaxPayloadSize;

  // EnableNetWarps(status == ServerStatus::online);
}

void RealPET::Homepage::InitializeFolderParticles()
{
  maxPoolSize = 12;
  pool.clear();
  pool.reserve(maxPoolSize);

  for (size_t i = 0; i < maxPoolSize; i++) {
    pool.emplace_back(Particle{});
  }
}

void RealPET::Homepage::InitializeWindowParticles()
{
  maxStaticPoolSize = 3;
  staticPool.clear();
  staticPool.reserve(maxStaticPoolSize);

  for (size_t i = 0; i < maxStaticPoolSize; i++) {
    staticPool.emplace_back(StaticParticle{});
    staticPool[i].startup_delay = static_cast<double>(rand() % (maxStaticPoolSize*3));
  }
}

void RealPET::Homepage::UpdateFolderParticles(double elapsed)
{

  sf::RenderWindow& window = getController().getWindow();
  sf::Vector2i mousei = sf::Mouse::getPosition(window);
  sf::Vector2f mousef = window.mapPixelToCoords(mousei);

  // Find particles that are dead or unborn
  for (Particle& p : pool) {
    if (p.lifetime > p.max_lifetime) {
      if (p.click) {
        mouseClicked = false; // free mouse
      }

      p = Particle{
        0, // lifetime starts at 0
        rand_val(8.0, 16.0), // duration
        rand_val(sf::Vector2f(-8, -15), sf::Vector2f(8, 8)), // acceleration
        rand_val(sf::Vector2f(-1, -1), sf::Vector2f(1, 1)), // velocity
        rand_val(sf::Vector2f(0.99f, 0.99f), sf::Vector2f(0.5f, 0.65f)), // friction
        rand_val(sf::Vector2f(-10.f, 80 * 2.f), sf::Vector2f(10.f + 240 * 2.f, 160 * 2.f)), // position
        rand_val(), // scaleIn
        rand_val(), // scaleOut
        rand_val(0, 2) // type
      };
    }

    // now update particles
    p.lifetime += elapsed;

    // fly away from the mouse
    sf::Vector2f v = p.position - mousef;
    float length = std::sqrtf(v.x * v.x + v.y * v.y);
    float max_dist = 50.f;

    if (length > 0.0f) {
      v.x = v.x / length;
      v.y = v.y / length;
    }

    bool canClick = !p.click.has_value() && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !mouseClicked;
    if (length < 10.0f && canClick) {
      p.click = p.position - mousef;
      mouseClicked = true;
    }

    if (p.click.has_value()) {
      // follow mouse
      p.position = *p.click + mousef;

      // release
      if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2f mouseDelta;

        for (sf::Vector2f m : lastMousef) {
          mouseDelta += mousef - m;
        }

        lastMousef.fill({});

        p.velocity = mouseDelta;
        p.acceleration = sf::Vector2f(p.velocity.x * elapsed, p.velocity.y * elapsed );
        p.click.reset();

        mouseClicked = false;
      }
      else {
        continue; // skip physics below
      }
    }

    float dropoff = std::clamp(1.0f - (length / max_dist), 0.f, 1.0f);

    double fleeSpeed = 5.f;
    p.velocity += sf::Vector2f(v.x * fleeSpeed * dropoff, v.y * fleeSpeed * dropoff);
    p.velocity += sf::Vector2f(p.acceleration.x * elapsed, p.acceleration.y * elapsed);

    p.position = p.position + sf::Vector2f(p.velocity.x * elapsed * p.friction.x, p.velocity.y * elapsed * p.friction.y);
  }

  if (mouseClicked) {
    lastMousef[(++mouseBufferIdx) % lastMousef.size()] = mousef;
  }
}

void RealPET::Homepage::UpdateWindowParticles(double elapsed)
{
  // Find particles that are dead or unborn
  for (StaticParticle& p : staticPool) {
    if (p.startup_delay > 0.0) {
      p.startup_delay -= elapsed;
      continue; // skip this particle
    }

    if (p.lifetime > p.max_lifetime) {
      p = StaticParticle{
        0, // lifetime starts at 0
        rand_val(3.0, 15.0), // duration
        rand_val(sf::Vector2f(60.5f * 2.0, 0), sf::Vector2f((240.f - 60.5f) * 2.f, 160 * 2.f)), // position
      };
    }

    // now update particles
    p.lifetime += elapsed;
  }
}

void RealPET::Homepage::DrawFolderParticles(sf::RenderTexture& surface)
{
  sf::Sprite particleSpr;
  particleSpr.setTexture(*folderTexture);

  for (Particle& p : pool) {
    if (p.lifetime > p.max_lifetime) continue;

    folderAnim.SetAnimation(std::to_string(p.type));
    folderAnim.Refresh(particleSpr);

    double beta = swoosh::ease::wideParabola(p.lifetime, p.max_lifetime, 1.0);

    bool scaleOut = p.lifetime > (p.max_lifetime * 0.5);
    bool scaleIn = !scaleOut;

    scaleOut = scaleOut && p.scaleOut;
    scaleIn = scaleIn && p.scaleIn;

    particleSpr.setPosition(p.position);

    if (scaleOut || scaleIn) {
      particleSpr.setScale(sf::Vector2f(beta * 2.f, beta * 2.f));
    }

    particleSpr.setColor(sf::Color(255, 255, 255, static_cast<int>(beta * 100)));

    surface.draw(particleSpr);
  }
}

void RealPET::Homepage::DrawWindowParticles(sf::RenderTexture& surface)
{
  sf::Sprite particleSpr;
  particleSpr.setTexture(*windowTexture);
  frame_time_t windowAnimDur = windowAnim.GetStateDuration("default");

  for (StaticParticle& p : staticPool) {
    if (p.startup_delay > 0.0) {
      continue; // skip this particle
    }

    frame_time_t lifetimeFrames = from_seconds(p.lifetime);
    if (lifetimeFrames > windowAnimDur) continue;

    windowAnim.SyncTime(lifetimeFrames);
    windowAnim.Refresh(particleSpr);
    particleSpr.setPosition(p.pos);
    particleSpr.setScale(2.f, 2.f);

    surface.draw(particleSpr);
  }
}

void RealPET::Homepage::onUpdate(double elapsed)
{
  if (IsInFocus()) {
    menuWidget.Update(elapsed);
  }

  UpdateFolderParticles(elapsed);
  UpdateWindowParticles(elapsed);

  if (Input().Has(InputEvents::pressed_confirm)) {
    playJackin = true;
    Audio().StopStream();
    Audio().Play(jackinsfx, AudioPriority::highest);

    jackinAnim << [this] {
      using tx = segue<WhiteWashFade, types::milli<500>>::to<Overworld::Homepage>;
      getController().push<tx>();
    };
  }

  if (playJackin) {
    jackinAnim.Update(elapsed, jackinSprite);
  }
}

void RealPET::Homepage::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bgSprite);

  DrawFolderParticles(surface);
  DrawWindowParticles(surface);

  if (playJackin) {
    surface.draw(jackinSprite);
  }
  else {
    surface.draw(menuWidget);
  }
}

void RealPET::Homepage::onStart()
{
  Audio().Stream(StreamPaths::REAL_PET, true);

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
  Audio().Stream(StreamPaths::REAL_PET, true);

  if (packetProcessor) {
    Net().AddHandler(remoteAddress, packetProcessor);
  }

  getController().Session().SaveSession(FilePaths::PROFILE);
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
  if (lastSelectedNaviId == currentNaviId) return;

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
    // TODO: menuWidget.EnqueueMessage("No enemy mods installed.");
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
