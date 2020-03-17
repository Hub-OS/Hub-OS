#include <string>
using std::to_string;

#include "bnBattleScene.h"
#include "bnPlayer.h"
#include "bnField.h"
#include "bnCannon.h"
#include "bnBasicSword.h"
#include "bnTile.h"
#include "bnEnemyCardsUI.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCard.h"
#include "bnEngine.h"

EnemyCardsUI::EnemyCardsUI(Character* _owner) : CardUsePublisher(), Component(_owner) {
  cardCount = curr = 0;
  icon = sf::Sprite();
  icon.setTextureRect({ 0,0,14,14 });
  icon.setScale(sf::Vector2f(2.f, 2.f));
  this->character = _owner;
}

EnemyCardsUI::~EnemyCardsUI() {
}

void EnemyCardsUI::OnUpdate(float _elapsed) {
  if (GetOwner() && GetOwner()->GetTile() && !GetOwner()->IsDeleted() && GetOwner()->IsBattleActive()) {
    Agent* agent = GetOwnerAs<Agent>();

    if (agent && agent->GetTarget() && !agent->GetTarget()->IsDeleted() && agent->GetTarget()->GetTile()) {
      if (agent->GetTarget()->GetTile()->GetY() == GetOwner()->GetTile()->GetY()) {
        if (rand() % 500 > 299) {
          this->UseNextCard();
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

void EnemyCardsUI::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
  sf::RenderStates this_states = states.transform * this->getTransform();
  if (character && character->GetHealth() > 0) {
    int cardOrder = 0;
    for (int i = curr; i < cardCount; i++) {
      icon.setTextureRect(sf::IntRect(0,0,14,14));

      sf::Vector2f offset = character->getPosition();
      icon.setPosition(offset + sf::Vector2f(((i - curr) * 2.0f) - 4.f, -58.0f - 63.f - (i - curr) * -2.0f));

      ENGINE.Draw(icon);
    }
  }

  SceneNode::draw(target, states);
}

void EnemyCardsUI::LoadCards(const std::vector<Battle::Card>& incoming) {
  selectedCards = incoming;
  cardCount = (int)incoming.size();
  curr = 0;
}

void EnemyCardsUI::UseNextCard() {
  if (curr >= cardCount) {
    return;
  }

  std::cout << "selected card " << selectedCards[curr].GetShortName() << " is broadcasted by enemy UI" << std::endl;
  this->Broadcast(selectedCards[curr], *this->character);

  curr++;
}

void EnemyCardsUI::Inject(BattleScene& scene)
{
  scene.Inject(*((CardUsePublisher*)this));
}
