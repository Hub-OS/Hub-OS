#include "bnGameOverScene.h"
#include "bnMainMenuScene.h"
#include "Segues/WhiteWashFade.h"
#include <Swoosh/ActivityController.h>

using namespace swoosh::types;

GameOverScene::GameOverScene(swoosh::ActivityController& controller) : swoosh::Activity(&controller) {
  fadeInCooldown = 2.0f;

  gameOver.setTexture(*TEXTURES.GetTexture(TextureType::GAME_OVER));
  gameOver.setScale(2.f, 2.f);
  gameOver.setOrigin(gameOver.getLocalBounds().width / 2, gameOver.getLocalBounds().height / 2);

  leave = false;
}

GameOverScene::~GameOverScene() {

}

void GameOverScene::onStart() {
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/game_over.ogg");
}

void GameOverScene::onResume() {

}

void GameOverScene::onUpdate(double elapsed) {
  if(!leave) {
    fadeInCooldown -= (float) elapsed;

    if (fadeInCooldown < 0) {
      fadeInCooldown = 0;
    }

    if (fadeInCooldown == 0 && (sf::Touch::isDown(0) || INPUTx.Has(EventTypes::PRESSED_CONFIRM))) {
      leave = true;
    }
  } else {
    fadeInCooldown += (float) elapsed;

    if (fadeInCooldown > 2.0f) {
      fadeInCooldown = 2.0f;

      leave = false;

      using effect = segue<WhiteWashFade, milliseconds<500>>;
      getController().push<effect::to<MainMenuScene>>(!WEBCLIENT.IsLoggedIn());
    }
  }
}

void GameOverScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  sf::Vector2f logoPos = (sf::Vector2f)((sf::Vector2i)getController().getVirtualWindowSize() / 2);
  gameOver.setPosition(logoPos);
  gameOver.setColor(sf::Color(255, 255, 255, (sf::Uint32)(255 * (1.0f-(fadeInCooldown / 2.f)))));
  ENGINE.Draw(gameOver);
}
