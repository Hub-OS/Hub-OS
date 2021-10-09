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
#include "bnPaletteSwap.h"

Megaman::Megaman() : Player() {
  auto basePallete = Textures().LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  PaletteSwap* pswap = CreateComponent<PaletteSwap>(this);
  pswap->SetBase(basePallete);

  SetHealth(200);
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

  SetEmotion(Emotion::angry);
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

CardAction* Megaman::OnExecuteBusterAction()
{
  return new BusterCardAction(this, false, 1*GetAttackLevel());
}

CardAction* Megaman::OnExecuteChargedBusterAction()
{
  if (activeForm) {
    return activeForm->OnChargedBusterAction(*this);
  }
  else {
    return new BusterCardAction(this, true, 10*GetAttackLevel());
  }
}

CardAction* Megaman::OnExecuteSpecialAction() {
  if (activeForm) {
    return activeForm->OnSpecialAction(*this);
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

void ForteCross::OnActivate(Player& player)
{
  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/forte_cross.png");

  prevTexture = player.getTexture();
  player.setTexture(cross);

  if (auto* animComponent = player.GetFirstComponent<AnimationComponent>()) {
    prevAnimation = animComponent->GetFilePath();
    prevState = animComponent->GetAnimationString();
    animComponent->SetPath("resources/navis/megaman/forms/forte_cross.animation");
    animComponent->Reload();
    animComponent->SetAnimation(prevState);
  }

  player.SetPalette(nullptr);

  player.SetAirShoe(true);
}

void ForteCross::OnDeactivate(Player& player)
{
  if (auto pswap = player.GetFirstComponent<PaletteSwap>()) {
    pswap->Enable(true);
  }

  if (auto* animComponent = player.GetFirstComponent<AnimationComponent>()) {
    animComponent->SetPath(prevAnimation);
    animComponent->Reload();
    animComponent->SetAnimation(prevState);
  }

  player.setTexture(prevTexture);
}

void ForteCross::OnUpdate(double elapsed, Player& player)
{
}

CardAction* ForteCross::OnChargedBusterAction(Player& player)
{
  return new MachGunCardAction(&player, 10 * player.GetAttackLevel());
}

CardAction* ForteCross::OnSpecialAction(Player& player)
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

void TenguCross::OnActivate(Player& player)
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

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  
  auto palette = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/tengu.palette.png");
  player.SetPalette(palette);

  player.AddNode(overlay);
  player.SetAirShoe(true);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);
}

void TenguCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromSyncList(sync);
}

void TenguCross::OnUpdate(double elapsed, Player& player)
{
}

CardAction* TenguCross::OnChargedBusterAction(Player& player)
{
  auto* action = new WindRackCardAction(&player, 20 * player.GetAttackLevel() + 40);
  auto fan = new SpriteProxyNode();
  fan->setTexture(overlay->getTexture());
  action->ReplaceRack(fan, fanAnimation);
  return action;
}

CardAction* TenguCross::OnSpecialAction(Player& player)
{
  // class TenguCross::SpecialAction is a CardAction implementation
  return new TenguCross::SpecialAction(&player);
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

void HeatCross::OnActivate(Player& player)
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

void HeatCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  player.SetElement(Element::none);

  parentAnim->RemoveFromSyncList(sync);

  if (auto pswap = player.GetFirstComponent<PaletteSwap>()) {
    pswap->Revert();
  }
}

void HeatCross::OnUpdate(double elapsed, Player& player)
{
}

CardAction* HeatCross::OnChargedBusterAction(Player& player)
{
  auto* action = new FireBurnCardAction(&player, FireBurn::Type::_2, 20 * player.GetAttackLevel() + 30);
  action->CrackTiles(false);
  return action;
}

CardAction* HeatCross::OnSpecialAction(Player& player)
{
  return player.Player::OnExecuteSpecialAction();
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
  statusGuard = new DefenseStatusGuard();
}

TomahawkCross::~TomahawkCross()
{
  delete overlay;
  delete statusGuard;
}

void TomahawkCross::OnActivate(Player& player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/hawk_cross.animation");
  overlayAnimation.Load();
  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/hawk_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();

  auto palette = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/hawk.palette.png");
  player.SetPalette(palette);

  player.AddNode(overlay);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);

  player.SetAirShoe(true);
  player.AddDefenseRule(statusGuard);
}

void TomahawkCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromSyncList(sync);
  player.SetAirShoe(false);

  player.RemoveDefenseRule(statusGuard);
}

void TomahawkCross::OnUpdate(double elapsed, Player& player)
{
}

CardAction* TomahawkCross::OnChargedBusterAction(Player& player)
{
  return new TomahawkSwingCardAction(&player, 20 * player.GetAttackLevel() + 40);
}

CardAction* TomahawkCross::OnSpecialAction(Player& player)
{
  return player.Player::OnExecuteSpecialAction();
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

void ElecCross::OnActivate(Player& player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/elec_cross.animation");
  overlayAnimation.Load();
  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/elec_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  
  auto palette = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/elec.palette.png");
  player.SetPalette(palette);

  player.SetElement(Element::elec);
  player.AddNode(overlay);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);
}

void ElecCross::OnDeactivate(Player& player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::none);

  parentAnim->RemoveFromSyncList(sync);
}

void ElecCross::OnUpdate(double elapsed, Player& player)
{
}

CardAction* ElecCross::OnChargedBusterAction(Player& player)
{
  auto* action = new LightningCardAction(&player, 20 * player.GetAttackLevel() + 40);
  action->SetStun(false);
  return action;
}

CardAction* ElecCross::OnSpecialAction(Player& player)
{
  return player.Player::OnExecuteSpecialAction();
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
TenguCross::SpecialAction::SpecialAction(Character* actor) : 
  CardAction(actor, "PLAYER_IDLE") {

  OverrideAnimationFrames({ FRAMES });
}

TenguCross::SpecialAction::~SpecialAction()
{
}

void TenguCross::SpecialAction::OnExecute(Character* user)
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

    auto wind = new Wind(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(*wind, startCol, 1);

    wind = new Wind(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(*wind, startCol, 2);

    wind = new Wind(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(*wind, startCol, 3);
  };

  AddAnimAction(1, onTrigger);
}

void TenguCross::SpecialAction::OnActionEnd()
{
}

void TenguCross::SpecialAction::OnAnimationEnd()
{
}