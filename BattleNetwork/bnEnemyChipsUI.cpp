#include <string>
using std::to_string;

#include "bnBattleScene.h"
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

EnemyChipsUI::EnemyChipsUI(Character* _owner) : ChipUsePublisher(), Component(_owner) {
  chipCount = curr = 0;
  icon = sf::Sprite(*TEXTURES.GetTexture(CHIP_ICONS));
  icon.setScale(sf::Vector2f(2.f, 2.f));
  this->character = _owner;
  srand(time((time_t)0));
}

EnemyChipsUI::~EnemyChipsUI() {
}

void EnemyChipsUI::Update(float _elapsed) {
  if (GetOwner() && GetOwner()->GetTile() && !GetOwner()->IsDeleted() && GetOwner()->IsBattleActive()) {
    Agent* agent = GetOwnerAs<Agent>();

    if (agent && agent->GetTarget() && !agent->GetTarget()->IsDeleted() && agent->GetTarget()->GetTile()) {
      if (agent->GetTarget()->GetTile()->GetY() == GetOwner()->GetTile()->GetY()) {
        if (rand() % 500 > 299) {
          this->UseNextChip();
        }
      }
    }
  }
  else if (GetOwner() && GetOwner()->IsDeleted()) {
    GetOwner()->FreeComponentByID(this->GetID());
    this->FreeOwner();
    std::cout << "owner is free" << std::endl;
  }
}

void EnemyChipsUI::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
  sf::RenderStates this_states = states.transform * this->getTransform();
  if (character && character->GetHealth() > 0) {
    int chipOrder = 0;
    for (int i = curr; i < chipCount; i++) {
      sf::IntRect iconSubFrame = TEXTURES.GetIconRectFromID(selectedChips[curr].GetIconID());
      icon.setTextureRect(iconSubFrame);

      sf::Vector2f offset = character->getPosition();
      icon.setPosition(offset + sf::Vector2f(((i - curr) * 2.0f) - 4.f, -58.0f - 63.f - (i - curr) * -2.0f));

      ENGINE.Draw(icon);
    }
  }

  SceneNode::draw(target, states);
}

void EnemyChipsUI::LoadChips(std::vector<Chip> incoming) {
  selectedChips = incoming;
  chipCount = (int)incoming.size();
  curr = 0;
}

void EnemyChipsUI::UseNextChip() {
  if (curr >= chipCount) {
    return;
  }

  std::cout << "selected chip " << selectedChips[curr].GetShortName() << " is broadcasted by enemy UI" << std::endl;
  this->Broadcast(selectedChips[curr], *this->character);

  curr++;
}

void EnemyChipsUI::Inject(BattleScene& scene)
{
  scene.Inject(*((ChipUsePublisher*)this));
}
