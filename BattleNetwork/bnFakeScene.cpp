#include "bnFakeScene.h"
#include <Swoosh/ActivityController.h>

#include "Segues/DiamondTileSwipe.h"

FakeScene::FakeScene(swoosh::ActivityController& controller, sf::Texture& snapshot) : swoosh::Activity(&controller) {
  this->snapshot = sf::Sprite(snapshot);
  triggered = false;
<<<<<<< HEAD

  //this->snapshot.scale((float)this->snapshot.getTexture()->getSize().x / rect.width,
  //                    (float)this->snapshot.getTexture()->getSize().y / rect.height);
=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

FakeScene::~FakeScene() {

}

void FakeScene::onStart() {
}

void FakeScene::onResume() {

}

<<<<<<< HEAD
=======
/**
 * @brief Immediately pop the activity with a pattern effect
 * @param elapsed in seconds
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
void FakeScene::onUpdate(double elapsed) {
  if (!triggered) {
    triggered = true;

    using pattern = DiamondTileSwipe<swoosh::intent::direction::right>;
    using segue = swoosh::intent::segue<pattern, swoosh::intent::sec<1>>;
    getController().queuePop<segue>();
  }
}

void FakeScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(snapshot);
}
