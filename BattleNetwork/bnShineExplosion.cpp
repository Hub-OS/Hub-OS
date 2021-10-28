#include "bnShineExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"

using sf::IntRect;

ShineExplosion::ShineExplosion() : Artifact()
{
  SetLayer(0);
  setTexture(Textures().LoadFromFile(TexturePaths::MOB_BOSS_SHINE));
  setScale(2.f, 2.f);
}

void ShineExplosion::Init() {
  Artifact::Init();

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath("resources/mobs/boss_shine.animation");
  animationComponent->Load();
  animationComponent->SetAnimation("SHINE", Animator::Mode::Loop);
  animationComponent->Refresh();
}

void ShineExplosion::OnUpdate(double _elapsed) {
  Entity::drawOffset.y= -this->GetHeight();
}

void ShineExplosion::OnDelete()
{
}

ShineExplosion::~ShineExplosion()
{
}