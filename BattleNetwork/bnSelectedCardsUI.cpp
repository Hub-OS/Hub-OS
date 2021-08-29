#include <string>
#include <memory>
#include <Swoosh/Ease.h>

#include "bnField.h"
#include "bnSelectedCardsUI.h"
#include "bnResourceHandle.h"
#include "bnTextureResourceManager.h"
#include "bnInputManager.h"
#include "bnCurrentTime.h"
#include "bnCard.h"
#include "bnCardAction.h"
#include "bnCardToActions.h"
#include "bnCardPackageManager.h"
#include "battlescene/bnBattleSceneBase.h"

using std::to_string;

SelectedCardsUI::SelectedCardsUI(Character* owner, CardPackageManager* packageManager) :
  packageManager(packageManager),
  CardActionUsePublisher(), 
  UIComponent(owner)
{
  cardCount = curr = 0;
  auto iconRect = sf::IntRect(0, 0, 14, 14);
  icon.setTextureRect(iconRect);
  icon.setScale(sf::Vector2f(2.f, 2.f));

  frame.setTexture(LOAD_TEXTURE(CHIP_FRAME));
  frame.setScale(sf::Vector2f(2.f, 2.f));

  firstFrame = true; // first time drawn, update positions
}

SelectedCardsUI::~SelectedCardsUI() {
}

void SelectedCardsUI::draw(sf::RenderTarget & target, sf::RenderStates states) const {
  if (this->IsHidden()) return;

  const Entity* owner = GetOwner();

  //auto this_states = states;
  //this_states.transform *= getTransform();
  states.transform *= getTransform();

  int cardOrder = 0;

  // i = curr so we only see the cards that are left
  for (int i = curr; i < cardCount; i++) {
    // The first card appears in front
    // But the trick is that we start at i which is a remainder of the whole list
    // We first draw the last card in the list down to i
    int drawOrderIndex = cardCount - i + curr - 1;

    // If stacked, the algorithm makes a jagged pattern that goes up and to the left:
    // x = ( ( i - curr ) * spacing ) - spacing
    // y = -121.f - (i - curr) * (-spacing )
    sf::Vector2f dest = owner->getPosition() + sf::Vector2f(((i - curr) * 4.0f) - 4.f, -(owner->GetHeight()*2.0f) - 20.f - (i - curr) * -4.0f);

    // interpolate
    icon.setPosition(dest);

    // The black border needs to sit 1 pixel outside of the icon
    // 1px * 2 (scale) = 2px
    frame.setPosition(icon.getPosition());
    frame.setPosition(frame.getPosition() - sf::Vector2f(2.f, 2.f));
    target.draw(frame, states);

    // Grab the ID of the card and draw that icon from the spritesheet
    std::shared_ptr<sf::Texture> texture;
    std::string id = selectedCards[drawOrderIndex]->GetUUID();
    if (packageManager && packageManager->HasPackage(id)) {
      texture = packageManager->FindPackageByID(id).GetIconTexture();
    }
    else {
      texture = WEBCLIENT.GetIconForCard(id);
    }

    icon.setTexture(texture);

    target.draw(icon, states);
  }

  UIComponent::draw(target, states);
};

void SelectedCardsUI::OnUpdate(double _elapsed) {
  elapsed = _elapsed;

  if (auto character = GetOwnerAs<Character>()) {
    if (character->IsDeleted()) {
      this->Hide();
    }
  }
}

void SelectedCardsUI::LoadCards(Battle::Card ** incoming, int size) {
  selectedCards = incoming;
  cardCount = size;
  curr = 0;
}

bool SelectedCardsUI::UseNextCard() {
  Character* owner = this->GetOwnerAs<Character>();

  if (!owner) return false;

  const std::vector<std::shared_ptr<CardAction>> actions = owner->AsyncActionList();
  bool hasNextCard = curr < cardCount;
  bool canUseCard = true;

  // We could be using an ability, just make sure one of these actions are not from a card
  // Cards cannot be used if another card is still active
  for (std::shared_ptr<CardAction> action : actions) {
    canUseCard = canUseCard && (action->GetLockoutGroup() != CardAction::LockoutGroup::card);
  }

  canUseCard = canUseCard && hasNextCard;

  if (!canUseCard) {
    return false;
  }

  auto card = selectedCards[curr];

  if (!card->IsBooster()) {
    card->MultiplyDamage(multiplierValue);
  }

  multiplierValue = 1; // reset 

  // add a peek event to the action queue
  owner->AddAction(PeekCardEvent{ this, packageManager }, ActionOrder::voluntary);
  return true;
}

void SelectedCardsUI::Broadcast(std::shared_ptr<CardAction> action)
{
  curr++;
  CardActionUsePublisher::Broadcast(action, CurrentTime::AsMilli());
}

std::optional<std::reference_wrapper<const Battle::Card>> SelectedCardsUI::Peek()
{
  if (cardCount > 0) {
    using RefType = std::reference_wrapper<const Battle::Card>;
    return std::optional<RefType>(std::ref(*selectedCards[curr]));
  }

  return {};
}

void SelectedCardsUI::HandlePeekEvent(Character* from)
{
  auto maybe_card = this->Peek();

  if (maybe_card.has_value()) {
    // convert meta data into a useable action object
    const Battle::Card& card = *maybe_card;

    // could act on metadata later:
    // from->OnCard(card)
     
    // prepare for this frame's action animation (we must be actionable)
    from->MakeActionable();

    if (CardAction* newActionPtr = CardToAction(card, from, packageManager)) {
      std::shared_ptr<CardAction> action;
      action.reset(newActionPtr);
      action->SetMetaData(card.props); // associate the meta with this action object
      this->Broadcast(action); // tell the rest of the subsystems
    }
  }
}

std::vector<std::string> SelectedCardsUI::GetUUIDList()
{
  std::vector<std::string> res;

  for (int i = curr; i < cardCount; i++) {
    res.push_back(selectedCards[i]->GetUUID());
  }

  return res;
}

const int SelectedCardsUI::GetCardCount() const
{
  return cardCount;
}

const int SelectedCardsUI::GetCurrentCardIndex() const
{
  return curr;
}

const unsigned SelectedCardsUI::GetMultiplier() const
{
  return multiplierValue;
}

Battle::Card** SelectedCardsUI::SelectedCardsPtrArray() const
{
  return selectedCards;
}

SpriteProxyNode& SelectedCardsUI::IconNode() const
{
  return icon;
}

SpriteProxyNode& SelectedCardsUI::FrameNode() const
{
  return frame;
}

void SelectedCardsUI::Inject(BattleSceneBase& scene) {
  scene.Inject(*this);
}

void SelectedCardsUI::SetMultiplier(unsigned mult)
{
  multiplierValue = mult;
}
