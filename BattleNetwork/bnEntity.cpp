#include "bnEntity.h"
#include "bnComponent.h"
#include "bnTile.h"
#include "bnField.h"
#include "Swoosh\Ease.h"

const Hit::Properties Entity::DefaultHitProperties{ Hit::recoil, Element::NONE, 3.0, nullptr };

Entity::Entity()
  : tile(nullptr),
  next(nullptr),
  previous(nullptr),
  field(nullptr),
  previousDirection(Direction::NONE),
  direction(Direction::NONE),
  team(Team::UNKNOWN),
  isBattleActive(false),
  deleted(false),
  passthrough(false),
  ownedByField(false),
  isSliding(false),
  element(Element::NONE),
  tileOffset(sf::Vector2f(0,0)) {
  slideTime = sf::milliseconds(200);
  elapsedSlideTime = 0;
}

Entity::~Entity() {
}

void Entity::Update(float _elapsed) {
  if (_elapsed <= 0)
    return;

  if (isSliding && this->next) {
    elapsedSlideTime += _elapsed;

    sf::Vector2f pos = this->slideStartPosition;
    sf::Vector2f tar = this->next->getPosition();
    tar = sf::Vector2f(tar.x, tar.y);

    float delta = swoosh::ease::linear((float)elapsedSlideTime, slideTime.asSeconds(), 1.0f);
    auto interpol = tar * delta + (pos*(1.0f - delta));
    this->tileOffset = interpol - pos;

    

    if(delta == 1.0f)
    {
      elapsedSlideTime = 0;
      Battle::Tile* prevTile = this->GetTile();
      this->tileOffset = sf::Vector2f(0, 0);

      this->AdoptNextTile();

      if (isSliding) {
        std::cout << "we are sliding" << std::endl;;

        if(this->GetTile()->GetX() < prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() - 1, this->GetTile()->GetY());
        }
        else if (this->GetTile()->GetX() > prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() + 1, this->GetTile()->GetY());
        }
        else if (this->GetTile()->GetY() > prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() + 1);
        }
        else if (this->GetTile()->GetY() < prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() - 1);
        }

        if (!(this->next && this->next->GetTeam() == this->GetTeam() && this->next->GetState() == TileState::ICE)) {
          // Conditions not met
          std::cout << "Conditions not met" << std::endl;
          isSliding = false;
          this->next = nullptr;
        }
      }
    }
  }

  if (IsDeleted()) {
    field->RemoveEntity(this);
  }
}

bool Entity::Move(Direction _direction) {
  bool moved = false;

  Battle::Tile* temp = tile;
  if (_direction == Direction::UP) {
    if (tile->GetY() - 1 > 0) {
      next = field->GetAt(tile->GetX(), tile->GetY() - 1);
      if (Teammate(next->GetTeam()) && CanMoveTo(next)) {
        ;
      }
      else {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::LEFT) {
    if (tile->GetX() - 1 > 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
      if (Teammate(next->GetTeam()) && CanMoveTo(next)) {
        ;
      }
      else {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::DOWN) {
    if (tile->GetY() + 1 <= (int)field->GetHeight()) {
      next = field->GetAt(tile->GetX(), tile->GetY() + 1);
      if (Teammate(next->GetTeam()) && CanMoveTo(next)) {
        ;
      }
      else {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::RIGHT) {
    if (tile->GetX() + 1 <= static_cast<int>(field->GetWidth())) {
      next = field->GetAt(tile->GetX() + 1, tile->GetY());
      if (Teammate(next->GetTeam()) && CanMoveTo(next)) {
        ;
      }
      else {
        next = nullptr;
      }
    }
  }

  if (next) {
    this->previousDirection = _direction;

    previous = temp;

    if (isSliding) return false;

    moved = true;
  }
  else {
    isSliding = false;
  }

  return moved;
}

bool Entity::CanMoveTo(Battle::Tile * next)
{
  return next? (this->HasFloatShoe()? true : next->IsWalkable()) : false;
}

vector<Drawable*> Entity::GetMiscComponents() {
  assert(false && "GetMiscComponents shouldn't be called directly from Entity");
  return vector<Drawable*>();
}

const float Entity::GetHitHeight() const {
  //assert(false && "GetHitHeight shouldn't be called directly from Entity");
  return 0;
}

const bool Entity::Hit(int damage, Hit::Properties props) {
  return false;
}

TextureType Entity::GetTextureType() {
  assert(false && "GetGetTextureType shouldn't be called directly from Entity");
  return (TextureType)0;
}

bool Entity::Teammate(Team _team) {
  return team == _team;
}

void Entity::SetTile(Battle::Tile* _tile) {
  tile = _tile;
}

Battle::Tile* Entity::GetTile() const {
  return tile;
}

void Entity::SlideToTile(bool enabled)
{
  isSliding = enabled;

  // capture potential slide starting position
  this->UpdateSlideStartPosition();
}

void Entity::SetField(Field* _field) {
  field = _field;
}

Field* Entity::GetField() const {
  return field;
}

Team Entity::GetTeam() const {
  return team;
}
void Entity::SetTeam(Team _team) {
  team = _team;
}

void Entity::SetPassthrough(bool state)
{
  passthrough = state;
}

bool Entity::IsPassthrough()
{
  return passthrough;
}

void Entity::SetFloatShoe(bool state)
{
  floatShoe = state;
}

bool Entity::HasFloatShoe()
{
  return floatShoe;
}

Direction Entity::GetPreviousDirection()
{
  return previousDirection;
}

void Entity::Delete()
{
  deleted = true;
}

bool Entity::IsDeleted() const {
  return deleted;
}

void Entity::SetElement(Element _elem)
{
  this->element = _elem;
}

const Element Entity::GetElement() const
{
  return element;
}

void Entity::AdoptNextTile()
{
  if (next == nullptr) return;

  if (next->GetState() == TileState::ICE && !this->HasFloatShoe()) {
    this->SlideToTile(true);
  }

  if (previous != nullptr) {
    previous->RemoveEntity(this);
    previous = nullptr;
  }

  SetTile(next);
  tile->AddEntity(this);

  next = nullptr;

  if (this->isSliding) {
    std::cout << "direction: " << (int)this->GetPreviousDirection() << std::endl;
    this->Move(this->GetPreviousDirection());

    // calculate our new entity's position in world coordinates based on next tile
    // It's the same in the update loop, but we need the value right at this moment
    this->UpdateSlideStartPosition();

    if (!next) this->SlideToTile(false);
  }

  moveCount++;
}

void Entity::SetBattleActive(bool state)
{
  isBattleActive = state;
}

void Entity::FreeComponents()
{
  for (int i = 0; i < shared.size(); i++) {
    shared[i]->FreeOwner();
  }
}

void Entity::RegisterComponent(Component* c) {
  shared.push_back(c);
}

void Entity::UpdateSlideStartPosition()
{
  slideStartPosition = sf::Vector2f(tileOffset.x + tile->getPosition().x, tileOffset.y + tile->getPosition().y);
}
