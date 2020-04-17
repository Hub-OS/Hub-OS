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
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  SetHealth(130);
  animationComponent->SetPlaybackSpeed(1.0);
  animationComponent->SetAnimation("IDLE");
  animationComponent->SetPlaybackMode(Animator::Mode::Loop);

  setTexture(TEXTURES.GetTexture(TextureType::MOB_HONEYBOMBER_ATLAS));
  setScale(2.f, 2.f);
  animationComponent->OnUpdate(0);
  this->RegisterComponent(animationComponent);

  shadow = new SpriteProxyNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->setPosition(-12.0f, 6.0f);
  this->AddNode(shadow);

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);
}

HoneyBomber::~HoneyBomber() {
  delete shadow;
}

void HoneyBomber::OnDelete() {
    if (virusBody) {
        this->RemoveDefenseRule(virusBody);
        delete virusBody;
        virusBody = nullptr;
    }

  this->ChangeState<ExplodeState<HoneyBomber>>();

  this->RemoveMeFromTurnOrder();
}

void HoneyBomber::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y - GetHeight());
  setPosition(getPosition() + tileOffset);

  this->AI<HoneyBomber>::Update(_elapsed);
}

const bool HoneyBomber::OnHit(const Hit::Properties props) {
  this->GetFirstComponent<AnimationComponent>()->SetPlaybackSpeed(2.0);
  return true;
}

const float HoneyBomber::GetHeight() const {
  return 30.0f;
}