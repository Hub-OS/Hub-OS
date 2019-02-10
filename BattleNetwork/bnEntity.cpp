#include "bnEntity.h"
#include "bnComponent.h"
#include "bnTile.h"
#include "bnField.h"
#include <Swoosh\Ease.h>

long Entity::numOfIDs = 0;

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
  tileOffset(sf::Vector2f(0,0)),
  slideTime(sf::milliseconds(100)),
  defaultSlideTime(slideTime),
  elapsedSlideTime(0)
{
  this->ID = ++Entity::numOfIDs;
}

Entity::~Entity() {
}

void Entity::Update(float _elapsed) {
  if (_elapsed <= 0)
    return;

  if (isSliding && this->next) {
    elapsedSlideTime += _elapsed;

    float delta = swoosh::ease::linear((float)elapsedSlideTime, slideTime.asSeconds(), 1.0f);

    sf::Vector2f pos = this->slideStartPosition;
    sf::Vector2f tar = this->next->getPosition();

    auto interpol = tar * delta + (pos*(1.0f - delta));
    this->tileOffset = interpol - pos;

    if (delta >= 0.5f) {
      if (tile != next) {
        previous->RemoveEntity(this);
        previous = tile;

        SetTile(next);
        tile->AddEntity(this);
      }

      this->tileOffset = -tar+pos+tileOffset;
    } 

    if(delta == 1.0f)
    {
      elapsedSlideTime = 0;
      Battle::Tile* prevTile = this->GetTile();
      this->tileOffset = sf::Vector2f(0, 0);

      if (isSliding) {
        // std::cout << "we are sliding" << std::endl;;

        //std::cout << "direction: " << (int)this->GetPreviousDirection() << std::endl;
        this->Move(this->GetPreviousDirection());

        // calculate our new entity's position in world coordinates based on next tile
        // It's the same in the update loop, but we need the value right at this moment
        this->UpdateSlideStartPosition();

        if(this->previous->GetX() < prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() - 1, this->GetTile()->GetY());
        }
        else if (this->previous->GetX() > prevTile->GetX()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX() + 1, this->GetTile()->GetY());
        }
        else if (this->previous->GetY() > prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() + 1);
        }
        else if (this->previous->GetY() < prevTile->GetY()) {
          this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() - 1);
        }

        if (!(this->next && Teammate(this->next->GetTeam()) && this->tile->GetState() == TileState::ICE)) {
          // Conditions not met
          //std::cout << "Conditions not met" << std::endl;
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

  //Direction testDirection = this->direction;
  this->direction = _direction;

  Battle::Tile* temp = tile;
  if (_direction == Direction::UP) {
    if (tile->GetY() - 1 > 0) {
      next = field->GetAt(tile->GetX(), tile->GetY() - 1);
      if (CanMoveTo(next)) {
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
      if (CanMoveTo(next)) {
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
      if (CanMoveTo(next)) {
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
      if (CanMoveTo(next)) {
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
    this->direction = Direction::NONE;
    isSliding = false;
  }

  return moved;
}

bool Entity::Teleport(int col, int row) {
  bool moved = false;

  //Direction testDirection = this->direction;
  Battle::Tile* temp = tile;

  this->direction = Direction::NONE;


  next = field->GetAt(col, row);

  if (CanMoveTo(next)) {
    ;
  }
  else {
    next = nullptr;
  }

  if (next) {
    this->previousDirection = Direction::NONE;

    previous = temp;

    moved = true;
  }

  isSliding = false;

  return moved;
}

bool Entity::CanMoveTo(Battle::Tile * next)
{
  bool valid = next? (this->HasFloatShoe()? true : next->IsWalkable()) : false;
  return valid && Teammate(next->GetTeam());
}

vector<Drawable*> Entity::GetMiscComponents() {
  assert(false && "GetMiscComponents shouldn't be called directly from Entity");
  return vector<Drawable*>();
}

TextureType Entity::GetTextureType() {
  assert(false && "GetGetTextureType shouldn't be called directly from Entity");
  return (TextureType)0;
}

const long Entity::GetID() const
{
  return this->ID;
}

bool Entity::Teammate(Team _team) {
  return (team == Team::UNKNOWN) || (team == _team);
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

void Entity::SetDirection(Direction dir) {
  this->direction = dir;
}

Direction Entity::GetDirection()
{
  return direction;
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

  moveCount++;
}

void Entity::SetBattleActive(bool state)
{
  isBattleActive = state;
}

void Entity::FreeAllComponents()
{
  for (int i = 0; i < shared.size(); i++) {
    shared[i]->FreeOwner();
  }
}

void Entity::FreeComponent(Component& c) {
  for (int i = 0; i < shared.size(); i++) {
    if (shared[i] == &c) {
      shared[i]->FreeOwner();
      shared.erase(shared.begin() + i);
      return;
    }
  }
}

void Entity::RegisterComponent(Component* c) {
  shared.push_back(c);
}

void Entity::UpdateSlideStartPosition()
{
  slideStartPosition = sf::Vector2f(tileOffset.x + tile->getPosition().x, tileOffset.y + tile->getPosition().y);
}
