#include "bnEntity.h"
#include "bnComponent.h"
#include "bnTile.h"
#include "bnField.h"
#include <Swoosh/Ease.h>

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
  elapsedSlideTime(0),
  lastComponentID(0)
{
  this->ID = ++Entity::numOfIDs;
  alpha = 255;
}

Entity::~Entity() {
  for (int i = 0; i < shared.size(); i++) {
    delete shared[i];
  }

  shared.clear();
}

void Entity::Update(float _elapsed) {
  for (int i = 0; i < shared.size(); i++) {
    shared[i]->Update(_elapsed);
  }


  if (_elapsed <= 0 || !tile)
    return;

  if (isSliding && this->next) {
    elapsedSlideTime += _elapsed;

    float delta = swoosh::ease::linear((float)elapsedSlideTime, slideTime.asSeconds(), 1.0f);

    sf::Vector2f pos = this->slideStartPosition;
    sf::Vector2f tar = this->next->getPosition();

    auto interpol = tar * delta + (pos*(1.0f - delta));
    this->tileOffset = interpol - pos;

    if (delta >= 0.5f) {
      // conditions may change, ensure by the time we switch
      if (this->CanMoveTo(next)) {
        if (tile != next) {
          previous->RemoveEntityByID(this->GetID());
          previous = tile;

          this->AdoptTile(next);
        }

        this->tileOffset = -tar + pos + tileOffset;
      }
      else {
        this->next = this->tile;
        //this->elapsedSlideTime = 0;
        //this->tileOffset = -tar + pos + tileOffset;
        //return; // end prematurely
      }
    } 

    if(delta == 1.0f)
    {
      elapsedSlideTime = 0;
      Battle::Tile* prevTile = this->GetTile();
      this->tileOffset = sf::Vector2f(0, 0);

      //if (isSliding) {
        // std::cout << "we are sliding" << std::endl;;

        //std::cout << "direction: " << (int)this->GetPreviousDirection() << std::endl;
        if (this->tile->GetState() == TileState::ICE && !this->HasFloatShoe()) {
          this->Move(this->GetPreviousDirection());


          // calculate our new entity's position in world coordinates based on next tile
          // It's the same in the update loop, but we need the value right at this moment
          this->UpdateSlideStartPosition();

          if (this->previous->GetX() > prevTile->GetX()) {
            this->next = this->GetField()->GetAt(this->GetTile()->GetX() - 1, this->GetTile()->GetY());
          }
          else if (this->previous->GetX() < prevTile->GetX()) {
            this->next = this->GetField()->GetAt(this->GetTile()->GetX() + 1, this->GetTile()->GetY());
          }
          else if (this->previous->GetY() < prevTile->GetY()) {
            this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() + 1);
          }
          else if (this->previous->GetY() > prevTile->GetY()) {
            this->next = this->GetField()->GetAt(this->GetTile()->GetX(), this->GetTile()->GetY() - 1);
          }

          if (((this->next && this->tile->GetState() != TileState::ICE) || !this->CanMoveTo(next))) {
            // Conditions not met
            //std::cout << "Conditions not met" << std::endl;
            isSliding = false;
            //this->AdoptNextTile();
          }
        }
        else {
          this->next = nullptr;
        }
      //}
    }
  }
  else {
    this->tileOffset = sf::Vector2f(0, 0);
    isSliding = false;
  }

  if (IsDeleted()) {
    if (tile) {
      tile->RemoveEntityByID(this->GetID());
    }

    //delete this;
  }
}

void Entity::SetAlpha(int value)
{
  alpha = value;
  sf::Color c = this->getColor();
  c.a = alpha;

  this->setColor(c);
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

  if (next && next != tile) {
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

  if (next && CanMoveTo(next)) {
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

const Battle::Tile* Entity::GetNextTile() const {
  return next;
}

void Entity::SlideToTile(bool enabled)
{
  isSliding = enabled;

  // capture potential slide starting position
  this->UpdateSlideStartPosition();
}

const bool Entity::IsSliding() const
{
  return isSliding;
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

void Entity::SetAirShoe(bool state) {
  airShoe = state;
}

bool Entity::HasFloatShoe()
{
  return floatShoe;
}

bool Entity::HasAirShoe() {
  return airShoe;
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
  if (next == nullptr) {
    return;
  }

  if (previous != nullptr) {
    previous->RemoveEntityByID(this->GetID());
    //previous = nullptr;
  }

  this->AdoptTile(next);

  if (next->GetState() == TileState::ICE && !this->HasFloatShoe()) {
    this->SlideToTile(true);
  }

  //next = nullptr;

  moveCount++;
}

void Entity::SetBattleActive(bool state)
{
  isBattleActive = state;
}

const bool Entity::IsBattleActive()
{
  return isBattleActive;
}

void Entity::FreeAllComponents()
{
  for (int i = 0; i < shared.size(); i++) {
    shared[i]->FreeOwner();
  }

  shared.clear();
}

void Entity::FreeComponentByID(long ID) {
  for (int i = 0; i < shared.size(); i++) {
    if (shared[i]->GetID() == ID) {
      shared[i]->FreeOwner();
      shared.erase(shared.begin() + i);
      return;
    }
  }
}

Component* Entity::RegisterComponent(Component* c) {
  shared.push_back(c);

  // Newest components appear first in the list for easy referencing
  std::sort(shared.begin(), shared.end(), [](Component* a, Component* b) { return a->GetID() > b->GetID(); });

  return c;
}

void Entity::UpdateSlideStartPosition()
{
  if (tile) {
    slideStartPosition = sf::Vector2f(tileOffset.x + tile->getPosition().x, tileOffset.y + tile->getPosition().y);
  }
}
