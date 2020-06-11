#include "bnEntity.h"
#include "bnComponent.h"
#include "bnTile.h"
#include "bnField.h"
#include <Swoosh/Ease.h>

long Entity::numOfIDs = 0;

// First entity ID begins at 1
Entity::Entity()
  : tile(nullptr),
  next(nullptr),
  previous(nullptr),
  field(nullptr),
  floatShoe(false),
  airShoe(false),
  previousDirection(Direction::none),
  direction(Direction::none),
  team(Team::unknown),
  isTimeFrozen(false),
  deleted(false),
  passthrough(false),
  ownedByField(false),
  isSliding(false),
  hasSpawned(false),
  flagForRemove(false),
  element(Element::none),
  tileOffset(sf::Vector2f(0, 0)),
  slideTime(sf::milliseconds(100)),
  defaultSlideTime(slideTime),
  elapsedSlideTime(0),
  lastComponentID(0),
  height(0),
  moveCount(0),
  alpha(255)
{
  ID = ++Entity::numOfIDs;
}

Entity::~Entity() {
}

void Entity::Spawn(Battle::Tile & start)
{
  if (!hasSpawned) {
    OnSpawn(start);
  }

  hasSpawned = true;
}

const float Entity::GetHeight() const {
  return height;
}

void Entity::SetHeight(const float height) {
  Entity::height = height;
}

const bool Entity::IsSuperEffective(Element _other) const {
  switch(GetElement()) {
    case Element::aqua:
        return _other == Element::elec;
        break;
    case Element::fire:
        return _other == Element::aqua;
        break;
    case Element::wood:
        return _other == Element::fire;
        break;
    case Element::elec:
        return _other == Element::wood;
        break;
  }
    
  return false;
}

void Entity::Update(float _elapsed) {
  // Update all components
  // May change the size of vector during update()
  auto copy = components;

  for (int i = 0; i < copy.size(); i++) {
    copy[i]->OnUpdate(_elapsed);
  }

  // Do not upate if the entity's current tile pointer is null
  if (_elapsed <= 0 || !tile)
    return;

  // Only slide if we have a valid next tile pointer
  if (isSliding && next) {
    elapsedSlideTime += _elapsed;

    // Get a value from 0.0 to 1.0
    float delta = swoosh::ease::linear((float)elapsedSlideTime, slideTime.asSeconds(), 1.0f);

    sf::Vector2f pos = slideStartPosition;
    sf::Vector2f tar = next->getPosition();

    // Interpolate the sliding position from the start position to the end position
    auto interpol = tar * delta + (pos*(1.0f - delta));
    tileOffset = interpol - pos;

    // Once halfway, the mmbn entities switch to the next tile
    // and the slide position offset must be readjusted 
    if (delta >= 0.5f) {
      // conditions of the target tile may change, ensure by the time we switch
      if (CanMoveTo(next)) {
        if (tile != next) {
          // Remove the entity from its previous tile pointer 
          previous->RemoveEntityByID(GetID());
          
          // Previous tile is now the current tile
          previous = tile;

          // Curent tile is now the next tile
          AdoptTile(next);
        }

        // Adjust for the new current tile, begin halfway approaching the current tile
        tileOffset = -tar + pos + tileOffset;
      }
      else {
        // Slide back into the origin tile if we can no longer slide to the next tile
        slideStartPosition = next->getPosition();
        next = tile;
      }
    } 

    // When delta is 1.0, the slide duration is complete
    if(delta == 1.0f)
    {
      elapsedSlideTime = 0;
      Battle::Tile* prevTile = GetTile();
      tileOffset = sf::Vector2f(0, 0);

      // If we slide onto an ice block and we don't have float shoe enabled, slide
      if (tile->GetState() == TileState::ice && !HasFloatShoe()) {
        // Move again in the same direction as before
        Move(GetPreviousDirection());

        // calculate our new entity's position
        UpdateSlideStartPosition();

        // TODO: shorten with GetPreviousDirection() switch
        if (previous->GetX() > prevTile->GetX()) {
          next = GetField()->GetAt(GetTile()->GetX() - 1, GetTile()->GetY());
        }
        else if (previous->GetX() < prevTile->GetX()) {
          next = GetField()->GetAt(GetTile()->GetX() + 1, GetTile()->GetY());
        }
        else if (previous->GetY() < prevTile->GetY()) {
          next = GetField()->GetAt(GetTile()->GetX(), GetTile()->GetY() + 1);
        }
        else if (previous->GetY() > prevTile->GetY()) {
          next = GetField()->GetAt(GetTile()->GetX(), GetTile()->GetY() - 1);
        }

        // If the next tile is not available, not ice, or we are ice element, don't slide
        if (((next && tile->GetState() != TileState::ice) 
          || (next && !CanMoveTo(next)))
          || (GetElement() == Element::ice)) {
          // Conditions not met
          isSliding = false;
        }
      }
      else {
        // Invalidate the next tile pointer
        next = nullptr;
      }
    }
  }
  else {
    // If we don't have a valid next tile pointer or are not sliding,
    // Keep centered in the current tile with no offset
    //tileOffset = sf::Vector2f(0, 0);
    isSliding = false;
  }
}

void Entity::SetAlpha(int value)
{
  alpha = value;
  sf::Color c = getColor();
  c.a = alpha;

  setColor(c);
}


bool Entity::Move(Direction _direction) {
  if (!tile) return false;

  bool moved = false;

  // Update the entity direction 
  direction = _direction;

  Battle::Tile* temp = tile;
  
  if (_direction == Direction::none) next = nullptr;

  // Based on the input direction grab the tile we wish 
  // to move to and check to see if this entity is allowed
  // to move onto it with CanMoveTo() 
  if (_direction == Direction::up) {
    if (tile->GetY() - 1 >= 0) {
      next = field->GetAt(tile->GetX(), tile->GetY() - 1);
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::left) {
    if (tile->GetX() - 1 >= 0) {
      next = field->GetAt(tile->GetX() - 1, tile->GetY());
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::down) {
    if (tile->GetY() + 1 <= (int)field->GetHeight()+1) {
      next = field->GetAt(tile->GetX(), tile->GetY() + 1);
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }
  else if (_direction == Direction::right) {
    if (tile->GetX() + 1 <= (int)(field->GetWidth()+1)) {
      next = field->GetAt(tile->GetX() + 1, tile->GetY());
      if (!CanMoveTo(next)) {
        next = nullptr;
      }
    }
  }

  // If the next tile pointer is valid and is different from our current tile, we are moving
  if (next && next != tile) {
    previousDirection = _direction;

    previous = temp;

    // If we are sliding onto this tile, prevent move callbacks by returning false
    if (isSliding) return false;

    moved = true;
  }
  else {
    direction = Direction::none;
    isSliding = false;
  }

  return moved;
}

bool Entity::Teleport(int col, int row) {
  bool moved = false;

  Battle::Tile* temp = tile;

  direction = Direction::none;


  next = field->GetAt(col, row);

  if (!(next && CanMoveTo(next))) {
    next = nullptr;
  }

  if (next) {
    previousDirection = Direction::none;

    previous = temp;

    moved = true;
  }

  isSliding = false;

  return moved;
}

// Default implementation of CanMoveTo() checks 
// 1) if the tile is walkable
// 2) if not, if the entity can float 
// 3) if the tile is valid and the next tile is the same team
bool Entity::CanMoveTo(Battle::Tile * next)
{
  bool valid = next? (HasFloatShoe()? true : next->IsWalkable()) : false;
  return valid && Teammate(next->GetTeam());
}

const long Entity::GetID() const
{
  return ID;
}

/** \brief Unkown team entities are friendly to all spaces @see Cubes */
bool Entity::Teammate(Team _team) {
  return (team == Team::unknown) || (team == _team);
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
  if (!isSliding) {
    isSliding = enabled;

    if (enabled) {
      // capture potential slide starting position
      UpdateSlideStartPosition();
    }
  }
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
  direction = dir;
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
  if (deleted) return;

  deleted = true;

  OnDelete();

  for (auto&& callbacks : removeCallbacks) {
    callbacks();
  }

  removeCallbacks.clear();
}

void Entity::Remove()
{
  flagForRemove = true;
}

std::reference_wrapper<Entity::RemoveCallback> Entity::CreateRemoveCallback()
{
  removeCallbacks.push_back(std::move(Entity::RemoveCallback()));
  return std::ref(*(removeCallbacks.end()-1));
}

bool Entity::IsDeleted() const {
  return deleted;
}

bool Entity::WillRemoveLater() const
{
    return flagForRemove;
}

void Entity::SetElement(Element _elem)
{
  element = _elem;
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
    previous->RemoveEntityByID(GetID());
  }

  AdoptTile(next);

  // Slide if the tile we are moving to is ICE
  if (next->GetState() == TileState::ice && !HasFloatShoe()) {
    SlideToTile(true);
  } else {
    // If not using animations, then 
    // adopting a tile is the last step in the move procedure
    // Increase the move count
    moveCount++;
  }
}

void Entity::ToggleTimeFreeze(bool state)
{
  isTimeFrozen = state;
}

const bool Entity::IsTimeFrozen()
{
  return isTimeFrozen;
}

void Entity::FreeAllComponents()
{
  for (int i = 0; i < components.size(); i++) {
    components[i]->FreeOwner();
  }

  components.clear();
}

void Entity::FreeComponentByID(Component::ID_t ID) {
  for (int i = 0; i < components.size(); i++) {
    if (components[i]->GetID() == ID) {
      components[i]->FreeOwner();
      components.erase(components.begin() + i);
      return;
    }
  }
}

void Entity::FinishMove()
{
  // prevent breaking sliding mechanics
  if (IsSliding()) return;

  next = nullptr;
}

Component* Entity::RegisterComponent(Component* c) {
  if (c == nullptr) return nullptr;

  auto iter = std::find(components.begin(), components.end(), c);
  if (iter != components.end())
    return *iter;

  components.push_back(c);

  // Newest components appear first in the list for easy referencing
  std::sort(components.begin(), components.end(), [](Component* a, Component* b) { return a->GetID() > b->GetID(); });

  return c;
}

void Entity::UpdateSlideStartPosition()
{
  if (tile) {
    slideStartPosition = sf::Vector2f(tileOffset.x + tile->getPosition().x, tileOffset.y + tile->getPosition().y);
  }
}

void Entity::SetSlideTime(sf::Time time) {
  slideTime = time;
}

const int Entity::GetMoveCount() const
{
    return moveCount;
}
