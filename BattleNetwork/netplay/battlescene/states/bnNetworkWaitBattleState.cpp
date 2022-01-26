#include "bnNetworkWaitBattleState.h"
#include "../bnNetworkBattleScene.h"
#include "../../battlescene/States/bnCardSelectBattleState.h"
#include "../../../bnPlayerControlledState.h"
#include "../../../bnPlayer.h"

NetworkWaitBattleState::~NetworkWaitBattleState()
{
}

void NetworkWaitBattleState::MarkReady()
{
  ready = true;
}

bool NetworkWaitBattleState::IsReady() const {
  return ready;
}

frame_time_t NetworkWaitBattleState::GetSyncFrame() const
{
  return syncFrame;
}

void NetworkWaitBattleState::SetSyncFrame(frame_time_t frame)
{
  syncFrame = frame;
}

void NetworkWaitBattleState::onStart(const BattleSceneState* next)
{
}

void NetworkWaitBattleState::onEnd(const BattleSceneState* next)
{
  ready = false;
}

void NetworkWaitBattleState::onUpdate(double elapsed)
{
  flicker += from_seconds(elapsed);
}

void NetworkWaitBattleState::onDraw(sf::RenderTexture& surface)
{
  if (flicker.count() % 60 > 30) {
    Text label = Text("Waiting...", Font::Style::thick);
    label.setScale(2.0f, 2.0f);
    label.setOrigin(label.GetLocalBounds().width, label.GetLocalBounds().height * 0.5f);


    sf::Vector2f position = sf::Vector2f(470.0f, 80.0f);;
    label.SetColor(sf::Color::Black);
    label.setPosition(position.x + 2.f, position.y + 2.f);
    surface.draw(label);
    label.SetColor(sf::Color::White);
    label.setPosition(position);
    surface.draw(label);
  }
}
