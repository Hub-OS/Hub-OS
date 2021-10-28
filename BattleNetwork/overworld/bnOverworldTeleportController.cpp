#define _USE_MATH_DEFINES
#include <cmath>

#include "bnOverworldTeleportController.h"
#include "bnOverworldActor.h"
#include "../bnAudioResourceManager.h"
#include "../bnShaderResourceManager.h"

Overworld::TeleportController::TeleportController()
{
  beam = std::make_shared<WorldSprite>();
  beam->setTexture(Textures().LoadFromFile("resources/ow/teleport.png"));
  beamAnim = Animation("resources/ow/teleport.animation");
  beamAnim.SetAnimation("teleport_in");
  beamAnim.SetFrame(0, beam->getSprite());
}

Overworld::TeleportController::Command& Overworld::TeleportController::TeleportOut(std::shared_ptr<Actor> actor)
{
  this->actor = actor;

  if (!teleportedOut) {
    actor->Hide();

    auto onStart = [=] {
      if (!mute) {
        Audio().Play(AudioType::AREA_GRAB);
      }
    };

    auto onFinish = [=] {
      this->animComplete = true;
    };

    animComplete = false;
    beamAnim << "TELEPORT_OUT" << Animator::On(1, onStart) << onFinish;
    beamAnim.Refresh(beam->getSprite());
    beam->Set3DPosition(actor->Get3DPosition());

    teleportedOut = true;
  }

  sequence.push(Command{ Command::state::teleport_out });
  return sequence.back();
}

Overworld::TeleportController::Command& Overworld::TeleportController::TeleportIn(std::shared_ptr<Actor> actor, const sf::Vector3f& start, Direction dir, bool doSpin)
{
  teleportedOut = false;

  auto onStart = [=] {
    if (!mute) {
      Audio().Play(AudioType::AREA_GRAB);
    }
  };

  auto onEnter = [=] {
    this->actor->Reveal();
    this->actor->SetShader(Shaders().GetShader(ShaderType::ADDITIVE));
    this->actor->setColor(sf::Color::Cyan);
    this->spin = doSpin;
    this->entered = true;
    this->spinProgress = 0;
  };

  auto onFinish = [=] {
    this->animComplete = true;
    this->actor->RevokeShader();
    this->actor->setColor(sf::Color::White);
  };

  this->walkFrames = frames(50);
  this->startDir = dir;
  this->actor = actor;
  this->animComplete = this->walkoutComplete = this->spin = false;
  this->beamAnim << "TELEPORT_IN" << Animator::On(2, onStart) << Animator::On(5, onEnter) << onFinish;

  this->beamAnim.Refresh(this->beam->getSprite());
  actor->Hide();
  actor->Set3DPosition(start);
  this->beam->Set3DPosition(start);

  this->sequence.push(Command{ Command::state::teleport_in, actor->GetWalkSpeed() });
  return this->sequence.back();
}

void Overworld::TeleportController::Update(double elapsed)
{
  if (sequence.empty()) return;

  this->beamAnim.Update(static_cast<float>(elapsed), beam->getSprite());
  this->beam->SetLayer(this->actor->GetLayer());

  auto& next = sequence.front();
  if (next.state == Command::state::teleport_in) {
    if (animComplete) {
      if (walkFrames > frames(0) && this->startDir != Direction::none) {
        // walk out for 50 frames
        actor->Walk(this->startDir, true);
        actor->SetWalkSpeed(40); // overwrite
        walkFrames -= from_seconds(elapsed);
      }
      else {
        this->walkoutComplete = true;
        actor->SetWalkSpeed(next.originalWalkSpeed);
        next.onFinish();
        sequence.pop();
      }
    }
    else if (entered) {
      constexpr float _2pi = static_cast<float>(2.0f * M_PI);
      constexpr float spin_frames = _2pi / 0.25f;
      float progress = spin_frames * static_cast<float>(elapsed);
      spinProgress += progress;

      auto cyan = sf::Color::Cyan;
      sf::Uint8 alpha = static_cast<sf::Uint8>(255 * (1.0f - (spinProgress / spin_frames)));
      this->actor->setColor({ cyan.r, cyan.g, cyan.b, alpha });

      if (spin) {
        actor->Face(Actor::MakeDirectionFromVector({ std::cos(spinProgress), std::sin(spinProgress) }));
      }
      else {
        actor->Face(this->startDir);
      }
    }
  }
  else {
    // Command::state::teleport_out
    if (animComplete) {
      next.onFinish();
      sequence.pop();
    }
  }
}

bool Overworld::TeleportController::TeleportedOut() const {
  return teleportedOut;
}

const bool Overworld::TeleportController::IsComplete() const
{
  if (sequence.empty()) return true;

  if (sequence.front().state == Command::state::teleport_in) {
    return walkoutComplete;
  }

  return animComplete;
}

std::shared_ptr<Overworld::WorldSprite> Overworld::TeleportController::GetBeam()
{
  return beam;
}

void Overworld::TeleportController::EnableSound(bool enable)
{
  mute = !enable;
}
