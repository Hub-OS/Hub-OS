#include "bnHoneyBomber.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseVirusBody.h"
#include "bnEngine.h"

const std::string RESOURCE_PATH = "resources/mobs/honeybomber/honeybomber.animation";

HoneyBomber::HoneyBomber(Rank _rank)
  : AI<HoneyBomber>(this), TurnOrderTrait<HoneyBomber>(), Character(_rank) {
  name = "HonyBmbr";
  SetTeam(Team::BLUE);

  SetElement(Element::WOOD);
  SetFloatShoe(true);

  auto animationComponent = new AnimationComponent(this);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();

  SetHealth(130);
  animationComponent->SetPlaybackSpeed(1.0);
  animationComponent->SetAnimation("IDLE");

  setTexture(*TEXTURES.GetTexture(TextureType::MOB_HONEYBOMBER_ATLAS));
  setScale(2.f, 2.f);
  animationComponent->OnUpdate(0);
  this->RegisterComponent(animationComponent);

  shadow = new SpriteSceneNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->setPosition(-16.0f, 10.0f);
  this->AddNode(shadow);

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);
}

HoneyBomber::~HoneyBomber() {
  delete shadow;
}

void HoneyBomber::OnDelete() {
  this->RemoveDefenseRule(virusBody);
  delete virusBody;

  this->ChangeState<ExplodeState<HoneyBomber>>();

  this->RemoveMeFromTurnOrder();
}

void HoneyBomber::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y - GetHitHeight());
  setPosition(getPosition() + tileOffset);

  this->AI<HoneyBomber>::Update(_elapsed);
}

const bool HoneyBomber::OnHit(const Hit::Properties props) {
  return true;
}

const float HoneyBomber::GetHitHeight() const {
  return 20.0f;
}