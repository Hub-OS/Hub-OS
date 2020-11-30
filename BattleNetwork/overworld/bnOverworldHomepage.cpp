#include <Segues/BlackWashFade.h>
#include "bnOverworldHomepage.h"
#include "bnOverworldOnlineArea.h"

using namespace swoosh::types;

Overworld::Homepage::Homepage(swoosh::ActivityController& controller, bool guestAccount) :
  guest(guestAccount),
  SceneBase(controller, guestAccount)
{
 
}

Overworld::Homepage::~Homepage()
{
}

void Overworld::Homepage::onUpdate(double elapsed)
{
  // Update our logic
  auto& map = GetMap();
  auto mousei = sf::Mouse::getPosition(*ENGINE.GetWindow());
  auto& [row, col] = map.PixelToRowCol(mousei);
  sf::Vector2f click = { (float)col * map.GetTileSize().x, (float)row * map.GetTileSize().y };

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Home)) {
    this->ResetMap();
    map.setScale(2.f, 2.f);
  }
  else if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) && !scaledmap) {
    map.setScale(map.getScale() * 1.25f);
    scaledmap = true;
  }
  else if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) && !scaledmap) {
    map.setScale(map.getScale() * 0.75f);
    scaledmap = true;
  }

  if (scaledmap
    && !sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp)
    && !sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown)) {
    scaledmap = false;
  }

  if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !clicked) {
    const size_t tilesetCount = map.GetTilesetItemCount();
    if (tilesetCount > 0) {
      auto tile = map.GetTileAt(click);
      tile.solid = false;

      tile.ID = ((++tile.ID) % tilesetCount) + 1ull;

      map.SetTileAt(click, tile);

      clicked = true;
    }
  }
  else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) && !clicked) {
    auto tile = map.GetTileAt(click);
    tile.solid = true;
    tile.ID = 0;

    map.SetTileAt(click, tile);

    clicked = true;
  }
  else if (clicked
    && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)
    && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
  {
    clicked = false;
  }

  // do default logic
  SceneBase::onUpdate(elapsed);
}

void Overworld::Homepage::onDraw(sf::RenderTexture& surface)
{
  SceneBase::onDraw(surface);
}

void Overworld::Homepage::onStart()
{
  SceneBase::onStart();

  // Stop any music already playing
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/loop_overworld.ogg", false);
}

const std::pair<bool, Overworld::Map::Tile**> Overworld::Homepage::FetchMapData()
{
  return Map::LoadFromFile(GetMap(), "resources/ow/homepage.txt");
}

void Overworld::Homepage::OnTileCollision(const Overworld::Map::Tile& tile)
{
  // on collision with warps
  static size_t next_warp = 0;
  auto* teleportController = &GetTeleportControler();
  
  if (tile.token == "W" && teleportController->IsComplete()) {
    auto& map = GetMap();
    auto* playerController = &GetPlayerController();
    auto* playerActor = &GetPlayer();

    auto warps = map.FindToken("W");

    if (warps.size()) {
      if (++next_warp >= warps.size()) {
        next_warp = 0;
      }

      auto teleportToNextWarp = [=] {
        auto finishTeleport = [=] {
          playerController->ControlActor(*playerActor);
        };

        auto& command = teleportController->TeleportIn(*playerActor, warps[next_warp], Direction::up);
        command.onFinish.Slot(finishTeleport);
      };

      playerController->ReleaseActor();
      auto& command = teleportController->TeleportOut(*playerActor);
      command.onFinish.Slot(teleportToNextWarp);
    }
  }

  if (tile.token == "N" && teleportController->IsComplete()) {
    auto& map = GetMap();
    auto* playerController = &GetPlayerController();
    auto* playerActor = &GetPlayer();

    auto teleportToCyberworld = [=] {
      // Calculate the origin by grabbing this tile's grid Y/X values
      auto idx = map.OrthoToRowCol(playerActor->getPosition());

      // return at the center origin of this tile
      sf::Vector2f returnPoint = sf::Vector2f(
        idx.second * map.GetTileSize().x  + (map.GetTileSize().x * 0.5f), 
        idx.first * map.GetTileSize().y + (map.GetTileSize().y * 0.5)
      );

      this->TeleportUponReturn(returnPoint);

      getController().push<segue<BlackWashFade>::to<Overworld::OnlineArea>>(guest);
    };

    playerController->ReleaseActor();
    auto& command = teleportController->TeleportOut(*playerActor);
    command.onFinish.Slot(teleportToCyberworld);
  }
}
