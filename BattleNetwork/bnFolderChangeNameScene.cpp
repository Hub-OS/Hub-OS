#include "bnFolderChangeNameScene.h"
#include "bnInputManager.h"
#include "Segues\BlackWashFade.h"
#include <Swoosh/ActivityController.h>

FolderChangeNameScene::FolderChangeNameScene(swoosh::ActivityController& controller) : swoosh::Activity(&controller) {
  leave = true;
  // folder menu graphic
  bg = sf::Sprite(LOAD_TEXTURE(FOLDER_CHANGE_NAME_BG));
  bg.setScale(2.f, 2.f);
}

FolderChangeNameScene::~FolderChangeNameScene() {

}

void FolderChangeNameScene::onStart() {
  leave = false; 
}

void FolderChangeNameScene::onUpdate(double elapsed) {
  if (!leave) {
    if (INPUT.Has(PRESSED_B)) {
      using namespace swoosh::intent;
      using effect = segue<BlackWashFade>;
      getController().queuePop<effect>();
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      leave = true;
    }
  }
}

void FolderChangeNameScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(bg);
}
