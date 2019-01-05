#include <string>
using std::to_string;

#include "bnPlayer.h"
#include "bnField.h"
#include "bnCannon.h"
#include "bnBasicSword.h"
#include "bnTile.h"
#include "bnEnemyChipsUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnChip.h"
#include "bnEngine.h"

EnemyChipsUI::EnemyChipsUI(Character* _owner) : ChipUsePublisher(), Component(_owner)
, owner(_owner) {
  chipCount = curr = 0;
  icon = sf::Sprite(*TEXTURES.GetTexture(CHIP_ICONS));
  icon.setScale(sf::Vector2f(2.f, 2.f));
}

EnemyChipsUI::~EnemyChipsUI(void) {
}

bool EnemyChipsUI::GetNextComponent(Drawable*& out) {
  static int i = 0;
  while (i < (int)components.size()) {
    out = components.at(i);
    i++;
    return true;
  }
  i = 0;
  return false;
}

void EnemyChipsUI::Update(float _elapsed) {
  if (owner && !owner->IsDeleted()) {
    // TODO: Move draw out of update. Utilize components.
    int chipOrder = 0;
    for (int i = curr; i < chipCount; i++) {
      icon.setPosition(owner->getPosition() + sf::Vector2f(((i - curr) * 2.0f) - 4.f, -58.0f - 63.f - (i - curr) * -2.0f));
      sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID(selectedChips[curr].GetIconID());
      icon.setTextureRect(iconSubFrame);
      ENGINE.Draw(icon);
    }
  }
}

void EnemyChipsUI::LoadChips(std::vector<Chip> incoming) {
  selectedChips = incoming;
  chipCount = incoming.size();
  curr = 0;
}

void EnemyChipsUI::UseNextChip() {
  if (curr >= chipCount) {
    return;
  }

  this->Broadcast(selectedChips[curr]);

  curr++;
}
