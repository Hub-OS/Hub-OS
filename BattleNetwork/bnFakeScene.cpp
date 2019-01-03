#include "bnFakeScene.h"
#include "Swoosh\ActivityController.h"

#include "Segues\DiamondTileSwipe.h"

FakeScene::FakeScene(swoosh::ActivityController& controller, sf::Texture& snapshot) : swoosh::Activity(controller) {
  this->snapshot = sf::Sprite(snapshot);
  triggered = false;
}

FakeScene::~FakeScene() {

}

void FakeScene::onStart() {
}

void FakeScene::onResume() {

}

void FakeScene::onUpdate(double elapsed) {
  if (!triggered) {
    triggered = true;

    using pattern = DiamondTileSwipe<swoosh::intent::direction::right>;
    using segue = swoosh::intent::segue<pattern, swoosh::intent::sec<3>>;
    getController().queuePop<segue>();
  }
}

void FakeScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(snapshot);
}