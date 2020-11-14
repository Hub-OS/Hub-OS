#include "bnFalzar.h"
#include "bnNaviExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseSuperArmor.h"
#include "bnEngine.h"
#include <numeric>

const std::string RESOURCE_PATH = "resources/mobs/falzar/falzar.animation";

Falzar::Falzar(Falzar::Rank rank) : 
  BossPatternAI<Falzar>(this),
  Character(rank) {
  SetName("Falzar");
  SetTeam(Team::blue);
  SetAirShoe(true);
  SetFloatShoe(true);

  animation = Animation(RESOURCE_PATH);

  if (GetRank() == Rank::EX) {
    SetHealth(6000);
  }
  else {
    SetHealth(1000);
  }

  setScale(2.f, 2.f);

  bossBody = new DefenseSuperArmor();
  AddDefenseRule(bossBody);

  // load spritesheet
  auto falzarTexture = TEXTURES.LoadTextureFromFile("resources/mobs/falzar/chicken_nuggets.png");

  // Prepare the nodes to use their portion of the atlas
  setTexture(falzarTexture);
  animation << "BODY";
  animation.Refresh(getSprite());
  //AddNode(&body);
  
  auto pntOrigin = animation.GetPoint("ORIGIN");
  auto pntGuardLeft = animation.GetPoint("GUARD_LEFT") - pntOrigin;
  auto pntGuardRight = animation.GetPoint("GUARD_RIGHT") - pntOrigin;
  auto pntHead = animation.GetPoint("HEAD") - pntOrigin;
  auto pntTail = animation.GetPoint("TAIL") - pntOrigin;
  auto pntLegRight = animation.GetPoint("LEG_RIGHT") - pntOrigin;
  auto pntLegLeft = animation.GetPoint("LEG_LEFT") - pntOrigin;

  head.setTexture(falzarTexture);
  animation << "HEAD_RESTING";
  animation.Refresh(head.getSprite());
  head.setPosition(pntHead);
  head.SetLayer(-1);
  head.EnableParentShader(true);
  AddNode(&head);

  // left size is in front of falzar (over)
  guardLeft.setTexture(falzarTexture);
  animation << "GUARD_LEFT";
  animation.Refresh(guardLeft.getSprite());
  guardLeft.setPosition(pntGuardLeft);
  guardLeft.SetLayer(-1);
  guardLeft.EnableParentShader(true);
  AddNode(&guardLeft);
  auto pntWingLeft = animation.GetPoint("WING");

  // right side is under falzar
  guardRight.setTexture(falzarTexture);
  animation << "GUARD_RIGHT";
  animation.Refresh(guardRight.getSprite());
  guardRight.setPosition(pntGuardRight);
  guardRight.SetLayer(1);
  guardRight.EnableParentShader(true);
  AddNode(&guardRight);
  auto pntWingRight = animation.GetPoint("WING");

  // left side is over
  wingLeft.setTexture(falzarTexture);
  animation << "WING_LEFT";
  animation.Refresh(wingLeft.getSprite());
  wingLeft.setPosition(pntWingLeft);
  wingLeft.SetLayer(-1);
  wingLeft.EnableParentShader(true);
  AddNode(&wingLeft);

  // right side is under falzar
  wingRight.setTexture(falzarTexture);
  animation << "WING_RIGHT";
  animation.Refresh(wingRight.getSprite());
  wingRight.setPosition(pntWingRight);
  wingRight.SetLayer(1);
  wingRight.EnableParentShader(true);
  AddNode(&wingRight);

  tail.setTexture(falzarTexture);
  animation << "TAIL";
  animation.Refresh(tail.getSprite());
  tail.setPosition(pntTail);
  tail.EnableParentShader(true);
  AddNode(&tail);

  legLeft.setTexture(falzarTexture);
  animation << "LEG_LEFT";
  animation.Refresh(legLeft.getSprite());
  legLeft.setPosition(pntLegLeft);
  legLeft.EnableParentShader(true);
  AddNode(&legLeft);

  legRight.setTexture(falzarTexture);
  animation << "LEG_RIGHT";
  animation.Refresh(legRight.getSprite());
  legRight.setPosition(pntLegRight);
  legRight.SetLayer(-1);
  legRight.EnableParentShader(true);
  AddNode(&legRight);

  shadow.setTexture(falzarTexture);
  animation << "SHADOW";
  animation.Refresh(shadow.getSprite());

  // Set the height relative to the body
  SetHeight(getSprite().getGlobalBounds().height);

  // Add boss pattern code
  AddState<FalzarIdleState>();
  //AddState<FalzarMoveState>(3);
  //AddState<FalzarRoarState>();
  //AddState<FalzarSpinState>(4);
}

Falzar::~Falzar()
{
  delete bossBody;
}


void Falzar::OnDelete() {
  RemoveDefenseRule(bossBody);
  InterruptState<NaviExplodeState<Falzar>>(10, 1.0f);
}

bool Falzar::CanMoveTo(Battle::Tile* next)
{
  return true;
}

void Falzar::OnUpdate(float _elapsed) {
  BossPatternAI<Falzar>::Update(_elapsed);

  setPosition(tile->getPosition() + tileOffset);
}