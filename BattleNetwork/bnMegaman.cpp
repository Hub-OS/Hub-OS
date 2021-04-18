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
#include "bnDefenseStatusGuard.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnWind.h"
#include "bnPaletteSwap.h"

Megaman::Megaman() : Player() {
  auto basePallete = Textures().LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  PaletteSwap* pswap = CreateComponent<PaletteSwap>(this, basePallete);

  SetHealth(900);
  SetName("Megaman");
  SetHeight(48.f);

  setTexture(Textures().GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/tengu_entry.png");
  AddForm<HeatCross>()->SetUIPath("resources/navis/megaman/forms/heat_entry.png");
  AddForm<ElecCross>()->SetUIPath("resources/navis/megaman/forms/elec_entry.png");
  AddForm<TomahawkCross>()->SetUIPath("resources/navis/megaman/forms/hawk_entry.png");
}

Megaman::~Megaman()
{
}

void Megaman::OnUpdate(double elapsed)
{
  Player::OnUpdate(elapsed);
}

CardAction* Megaman::OnExecuteBusterAction()
{
  return new BusterCardAction(*this, false, 1*GetAttackLevel());
}

CardAction* Megaman::OnExecuteChargedBusterAction()
{
  if (activeForm) {
    return activeForm->OnChargedBusterAction(*this);
  }
  else {
    return new BusterCardAction(*this, true, 10*GetAttackLevel());
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

// class TenguCross

TenguCross::TenguCross()
{
  loaded = false;
}

TenguCross::~TenguCross()
{
  delete overlay;
}

void TenguCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/tengu_cross.animation");
  overlayAnimation.Load();

  fanAnimation = overlayAnimation;
  fanAnimation.SetAnimation("FAN");

  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/tengu_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/tengu.palette.png");

  loaded = true;

  OnUpdate(0, player);

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
  overlay->setColor(player.getColor());
}

CardAction* TenguCross::OnChargedBusterAction(Player& player)
{
  auto* action = new WindRackCardAction(player, 20 * player.GetAttackLevel() + 40);
  auto fan = new SpriteProxyNode();
  fan->setTexture(overlay->getTexture());
  action->ReplaceRack(fan, fanAnimation);
  return action;
}

CardAction* TenguCross::OnSpecialAction(Player& player)
{
  // class TenguCross::SpecialAction is a CardAction implementation
  return new TenguCross::SpecialAction(player);
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
  loaded = false;
}

HeatCross::~HeatCross()
{
  delete overlay;
}

void HeatCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/heat_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/heat_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/heat.palette.png");

  player.SetElement(Element::fire);

  loaded = true;

  OnUpdate(0, player);
  player.AddNode(overlay);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);
}

void HeatCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::none);

  parentAnim->RemoveFromSyncList(sync);

}

void HeatCross::OnUpdate(double elapsed, Player& player)
{
  overlay->setColor(player.getColor());
}

CardAction* HeatCross::OnChargedBusterAction(Player& player)
{
  auto* action = new FireBurnCardAction(player, FireBurn::Type::_2, 20 * player.GetAttackLevel() + 30);
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
  loaded = false;
  statusGuard = new DefenseStatusGuard();
}

TomahawkCross::~TomahawkCross()
{
  delete overlay;
  delete statusGuard;
}

void TomahawkCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/hawk_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/hawk_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/hawk.palette.png");

  loaded = true;

  OnUpdate(0, player);
  player.AddNode(overlay);

  sync.anim = &overlayAnimation;
  sync.node = overlay;
  sync.point = "Head";

  parentAnim->AddToSyncList(sync);

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
  overlay->setColor(player.getColor());
}

CardAction* TomahawkCross::OnChargedBusterAction(Player& player)
{
  return new TomahawkSwingCardAction(player, 20 * player.GetAttackLevel() + 40);
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
  loaded = false;
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
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/elec.palette.png");

  player.SetElement(Element::elec);

  loaded = true;

  OnUpdate(0, player);
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
  overlay->setColor(player.getColor());
}

CardAction* ElecCross::OnChargedBusterAction(Player& player)
{
  auto* action = new LightningCardAction(player, 20 * player.GetAttackLevel() + 40);
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
TenguCross::SpecialAction::SpecialAction(Character& owner) : 
  CardAction(owner, "PLAYER_IDLE") {

  OverrideAnimationFrames({ FRAMES });
}

TenguCross::SpecialAction::~SpecialAction()
{
}

void TenguCross::SpecialAction::OnExecute()
{
  auto owner = &GetCharacter();
  auto team = owner->GetTeam();
  auto field = owner->GetField();

  auto onTrigger = [this, team, owner, field]() -> void {
    Direction direction{ Direction::left };

    if (team == Team::blue) {
      direction = Direction::right;
    }

    auto wind = new Wind(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(*wind, 6, 1);

    wind = new Wind(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(*wind, 6, 2);

    wind = new Wind(team);
    wind->SetDirection(direction);
    wind->DeleteOnTeamTile();
    field->AddEntity(*wind, 6, 3);
  };

  AddAnimAction(1, onTrigger);
}

void TenguCross::SpecialAction::OnEndAction()
{
}

void TenguCross::SpecialAction::OnAnimationEnd()
{
}