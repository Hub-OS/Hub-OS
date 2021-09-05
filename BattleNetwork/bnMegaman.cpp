#include "bnMegaman.h"
#include "bnField.h"
#include "bnShaderResourceManager.h"
#include "bnBusterCardAction.h"
#include "bnCrackShotCardAction.h"
#include "bnWindRackCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnTornadoCardAction.h"
#include "bnLightningCardAction.h"
#include "bnTomahawkSwingCardAction.h"
#include "bnMachGunCardAction.h"
#include "bnDefenseStatusGuard.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnWind.h"

Megaman::Megaman() : Player() {
  basePalette = Textures().LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  StoreBasePalette(basePalette);
  SetPalette(basePalette);

  SetHealth(1000);
  SetName("Megaman");
  SetHeight(48.f);

  std::string path = "resources/navis/megaman/navi_megaman_atlas.png";
  setTexture(Textures().LoadTextureFromFile(path));

  AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/tengu_entry.png");
  AddForm<HeatCross>()->SetUIPath("resources/navis/megaman/forms/heat_entry.png");
  AddForm<ElecCross>()->SetUIPath("resources/navis/megaman/forms/elec_entry.png");
  AddForm<TomahawkCross>()->SetUIPath("resources/navis/megaman/forms/hawk_entry.png");
  AddForm<ForteCross>()->SetUIPath("resources/navis/megaman/forms/forte_entry.png");

  // First sprite on the screen should be default player stance
  SetAnimation("PLAYER_IDLE");
}

Megaman::~Megaman()
{
}

void Megaman::OnUpdate(double elapsed)
{
  Player::OnUpdate(elapsed);
}

void Megaman::OnSpawn(Battle::Tile& start)
{
  if (ResourceHandle::Shaders().IsEnabled()) return;

  std::string path = "resources/navis/megaman/navi_megaman_atlas.og.png";
  setTexture(Textures().LoadTextureFromFile(path));
}

std::shared_ptr<CardAction> Megaman::OnExecuteBusterAction()
{
  return std::make_shared<BusterCardAction>(shared_from_base<Character>(), false, 1*GetAttackLevel());
}

std::shared_ptr<CardAction> Megaman::OnExecuteChargedBusterAction()
{
  if (activeForm) {
    return activeForm->OnChargedBusterAction(shared_from_base<Player>());
  }
  else {
    return std::make_shared<BusterCardAction>(shared_from_base<Character>(), true, 10*GetAttackLevel());
  }
}

std::shared_ptr<CardAction> Megaman::OnExecuteSpecialAction() {
  if (activeForm) {
    return activeForm->OnSpecialAction(shared_from_base<Player>());
  }

  return Player::OnExecuteSpecialAction();
}

///////////////////////////////////////////////
//            CROSSES / FORMS                //
///////////////////////////////////////////////

// class ForteCross

ForteCross::ForteCross()
{
}

ForteCross::~ForteCross()
{
}

void ForteCross::OnActivate(std::shared_ptr<Player> player)
{
  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/forte_cross.png");

  prevTexture = player->getTexture();
  player->setTexture(cross);

  if (auto animComponent = player->GetFirstComponent<AnimationComponent>()) {
    prevAnimation = animComponent->GetFilePath();
    prevState = animComponent->GetAnimationString();
    animComponent->SetPath("resources/navis/megaman/forms/forte_cross.animation");
    animComponent->Reload();
    animComponent->SetAnimation(prevState);
  }

  player.SetPalette(nullptr);
  player.SetAirShoe(true);
}

void ForteCross::OnDeactivate(std::shared_ptr<Player> player)
{
  if (auto* animComponent = player.GetFirstComponent<AnimationComponent>()) {
    animComponent->SetPath(prevAnimation);
    animComponent->Reload();
    animComponent->SetAnimation(prevState);
  }

  player.setTexture(prevTexture);
  player.SetPalette(player.GetBasePalette());
}

void ForteCross::OnUpdate(double elapsed, std::shared_ptr<Player> player)
{
}

std::shared_ptr<CardAction> ForteCross::OnChargedBusterAction(std::shared_ptr<Player> player)
{
  return std::make_shared<MachGunCardAction>(player, 10 * player->GetAttackLevel());
}

std::shared_ptr<CardAction> ForteCross::OnSpecialAction(std::shared_ptr<Player> player)
{
  // class ForteCross::SpecialAction is a CardAction implementation
  return nullptr;
}

frame_time_t ForteCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * 1 - 100i
  * 2 - 90i
  * 3 - 80i
  * 4 - 75i
  * 5 - 70i
  */
  switch (chargeLevel) {
  case 1:
    return frames(90);
  case 2:
    return frames(80);
  case 3:
    return frames(70);
  case 4:
    return frames(65);
  }

  return frames(60);
}

// class TenguCross

TenguCross::TenguCross()
{
}

TenguCross::~TenguCross()
{
  delete overlay;
}

void TenguCross::OnActivate(std::shared_ptr<Player> player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/tengu_cross.animation");
  overlayAnimation.Load();

  fanAnimation = overlayAnimation;
  fanAnimation.SetAnimation("FAN");

  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/tengu_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player->GetFirstComponent<AnimationComponent>();
  
  auto palette = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/tengu.palette.png");
  player.SetPalette(palette);

  player->AddNode(overlay);
  player->SetAirShoe(true);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);
}

void TenguCross::OnDeactivate(std::shared_ptr<Player> player)
{
  player.RemoveNode(overlay);
  player.SetPalette(player.GetBasePalette());
  parentAnim->RemoveFromSyncList(sync);
}

void TenguCross::OnUpdate(double elapsed, std::shared_ptr<Player> player)
{
}

std::shared_ptr<CardAction> TenguCross::OnChargedBusterAction(std::shared_ptr<Player> player)
{
  auto action = std::make_shared<WindRackCardAction>(player, 20 * player->GetAttackLevel() + 40);
  auto fan = new SpriteProxyNode();
  fan->setTexture(overlay->getTexture());
  action->ReplaceRack(fan, fanAnimation);
  return action;
}

std::shared_ptr<CardAction> TenguCross::OnSpecialAction(std::shared_ptr<Player> player)
{
  // class TenguCross::SpecialAction is a CardAction implementation
  return std::make_shared<TenguCross::SpecialAction>(player);
}

frame_time_t TenguCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * 1 - 100i
  * 2 - 90i
  * 3 - 80i
  * 4 - 75i
  * 5 - 70i
  */
  switch (chargeLevel) {
  case 1:
    return frames(90);
  case 2:
    return frames(80);
  case 3:
    return frames(70);
  case 4:
    return frames(65);
  }

  return frames(60);
}

// class HeatCross

HeatCross::HeatCross()
{
}

HeatCross::~HeatCross()
{
  delete overlay;
}

void HeatCross::OnActivate(std::shared_ptr<Player> player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/heat_cross.animation");
  overlayAnimation.Load();
  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/heat_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();

  auto palette = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/heat.palette.png");
  player.SetPalette(palette);
  player.SetElement(Element::fire);
  player.AddNode(overlay);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);
}

void HeatCross::OnDeactivate(std::shared_ptr<Player> player)
{
  player.RemoveNode(overlay);
  player.SetElement(Element::none);
  player.SetPalette(player.GetBasePalette());
  parentAnim->RemoveFromSyncList(sync);
}

void HeatCross::OnUpdate(double elapsed, std::shared_ptr<Player> player)
{
}

std::shared_ptr<CardAction> HeatCross::OnChargedBusterAction(std::shared_ptr<Player> player)
{
  auto action = std::make_shared<FireBurnCardAction>(player, FireBurn::Type::_2, 20 * player->GetAttackLevel() + 30);
  action->CrackTiles(false);
  return action;
}

std::shared_ptr<CardAction> HeatCross::OnSpecialAction(std::shared_ptr<Player> player)
{
  return player->Player::OnExecuteSpecialAction();
}

frame_time_t HeatCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * charge 1 - 70i
  * 2 - 60i
  * 3 - 50i
  * 4 - 45i
  * 5 - 40i
  */
  switch (chargeLevel) {
  case 1:
    return frames(60);
  case 2:
    return frames(50);
  case 3:
    return frames(40);
  case 4:
    return frames(35);
  }

  return frames(30);
}

// class TomahawkCross

TomahawkCross::TomahawkCross()
{
  statusGuard = std::make_shared<DefenseStatusGuard>();
}

TomahawkCross::~TomahawkCross()
{
  delete overlay;
}

void TomahawkCross::OnActivate(std::shared_ptr<Player> player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/hawk_cross.animation");
  overlayAnimation.Load();
  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/hawk_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player->GetFirstComponent<AnimationComponent>();

  auto palette = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/hawk.palette.png");
  player.SetPalette(palette);

  player->AddNode(overlay);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);

  player->SetAirShoe(true);
  player->AddDefenseRule(statusGuard);
}

void TomahawkCross::OnDeactivate(std::shared_ptr<Player> player)
{
  player.RemoveNode(overlay);
  player.SetAirShoe(false);
  player.RemoveDefenseRule(statusGuard);
  player.SetPalette(player.GetBasePalette());
  parentAnim->RemoveFromSyncList(sync);
}

void TomahawkCross::OnUpdate(double elapsed, std::shared_ptr<Player> player)
{
}

std::shared_ptr<CardAction> TomahawkCross::OnChargedBusterAction(std::shared_ptr<Player> player)
{
  return std::make_shared<TomahawkSwingCardAction>(player, 20 * player->GetAttackLevel() + 40);
}

std::shared_ptr<CardAction> TomahawkCross::OnSpecialAction(std::shared_ptr<Player> player)
{
  return player->Player::OnExecuteSpecialAction();
}

frame_time_t TomahawkCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * 1 - 120i
  * 2 - 110i
  * 3 - 100i
  * 4 - 95i
  * 5 - 90i
  */

  switch (chargeLevel) {
  case 1:
    return frames(110);
  case 2:
    return frames(100);
  case 3:
    return frames(90);
  case 4:
    return frames(85);
  }

  return frames(80);
}

// class ElecCross

ElecCross::ElecCross()
{
}

ElecCross::~ElecCross()
{
  delete overlay;
}

void ElecCross::OnActivate(std::shared_ptr<Player> player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/elec_cross.animation");
  overlayAnimation.Load();
  auto cross = player->Textures().LoadTextureFromFile("resources/navis/megaman/forms/elec_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player->GetFirstComponent<AnimationComponent>();
  
  auto palette = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/elec.palette.png");
  player.SetPalette(palette);

  player->SetElement(Element::elec);
  player->AddNode(overlay);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);
}

void ElecCross::OnDeactivate(std::shared_ptr<Player> player)
{
  player.RemoveNode(overlay);
  player.SetElement(Element::none);
  player.SetPalette(player.GetBasePalette());
  parentAnim->RemoveFromSyncList(sync);
}

void ElecCross::OnUpdate(double elapsed, std::shared_ptr<Player> player)
{
}

std::shared_ptr<CardAction> ElecCross::OnChargedBusterAction(std::shared_ptr<Player> player)
{
  auto action = std::make_shared<LightningCardAction>(player, 20 * player->GetAttackLevel() + 40);
  action->SetStun(false);
  return action;
}

std::shared_ptr<CardAction> ElecCross::OnSpecialAction(std::shared_ptr<Player> player)
{
  return player->Player::OnExecuteSpecialAction();
}

frame_time_t ElecCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These numbers include the 10i+ startup frames
  * 1 - 90i
  * 2 - 80i
  * 3 - 70i
  * 4 - 65i
  * 5 - 60i
  */

  switch (chargeLevel) {
  case 1:
    return frames(80);
  case 2:
    return frames(70);
  case 3:
    return frames(60);
  case 4:
    return frames(55);
  }

  return frames(50);
}

////////////////////////////////////////////////////////////////
//             SPECIAL ABILITY IMPLEMENTATIONS                //
////////////////////////////////////////////////////////////////

#define FRAME1 { 1, 0.4 }

#define FRAMES FRAME1

// class TenguCross
TenguCross::SpecialAction::SpecialAction(std::shared_ptr<Character> actor) : 
  CardAction(actor, "PLAYER_IDLE") {
  OverrideAnimationFrames({ FRAMES });
}

TenguCross::SpecialAction::~SpecialAction()
{
}

void TenguCross::SpecialAction::OnExecute(std::shared_ptr<Character> user)
{
  auto team = user->GetTeam();
  auto field = user->GetField();

  auto onTrigger = [=]() -> void {
    int startCol = 6;
    Direction direction{ Direction::left };

    if (team == Team::blue) {
      direction = Direction::right;
      startCol = 1;
    }

    auto wind = std::make_shared<Wind>(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(wind, startCol, 1);

    wind = std::make_shared<Wind>(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(wind, startCol, 2);

    wind = std::make_shared<Wind>(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(wind, startCol, 3);
  };

  AddAnimAction(1, onTrigger);
}

void TenguCross::SpecialAction::OnActionEnd()
{
}

void TenguCross::SpecialAction::OnAnimationEnd()
{
}