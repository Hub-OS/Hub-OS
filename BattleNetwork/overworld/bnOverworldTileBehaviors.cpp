#include "bnOverworldTileBehaviors.h"

#include "bnOverworldActor.h"
#include "bnOverworldActorPropertyAnimator.h"
#include "../bnMath.h"

constexpr float DEFAULT_CONVEYOR_SPEED = 6.0f;
constexpr float DEFAULT_ICE_SPEED = 6.0f;
constexpr float DEFAULT_TREADMILL_SPEED = 1.875f;

void Overworld::TileBehaviors::UpdateActor(SceneBase& scene, Actor& actor, ActorPropertyAnimator& propertyAnimator) {
  if (propertyAnimator.IsAnimatingPosition()) {
    // if the something else is dragging the player around, ignore tiles
    return;
  }

  auto& map = scene.GetMap();
  auto layerIndex = actor.GetLayer();

  if (layerIndex < 0 || layerIndex >= map.GetLayerCount()) {
    return;
  }

  auto actorTilePos = map.WorldToTileSpace(actor.getPosition());
  auto& layer = map.GetLayer(layerIndex);
  auto tile = layer.GetTile(int(actorTilePos.x), int(actorTilePos.y));

  if (!tile) {
    return;
  }

  auto tileMeta = map.GetTileMeta(tile->gid);

  if (!tileMeta) {
    return;
  }

  switch (tileMeta->type) {
  case TileType::conveyor:
    HandleConveyor(scene, actor, propertyAnimator, *tileMeta, *tile);
    break;
  case TileType::ice:
    HandleIce(scene, actor, propertyAnimator, *tileMeta, *tile);
    break;
  case TileType::treadmill:
    HandleTreadmill(scene, actor, propertyAnimator, *tileMeta, *tile);
    break;
  }
}

void Overworld::TileBehaviors::HandleConveyor(SceneBase& scene, Actor& actor, ActorPropertyAnimator& propertyAnimator, TileMeta& tileMeta, Tile& tile) {
  auto& map = scene.GetMap();
  auto& layer = map.GetLayer(actor.GetLayer());
  auto direction = tileMeta.direction;

  if (tile.flippedHorizontal) {
    direction = FlipHorizontal(direction);
  }

  if (tile.flippedVertical) {
    direction = FlipVertical(direction);
  }

  auto startTilePos = map.WorldToTileSpace(actor.getPosition());
  auto endTilePos = startTilePos;
  float tileDistance = 0.0f;

  ActorPropertyAnimator::PropertyStep animationProperty;
  animationProperty.property = ActorProperty::animation;

  // resolve animation
  switch (direction) {
  case Direction::up_left:
    animationProperty.stringValue = "IDLE_UL";
    break;
  case Direction::up_right:
    animationProperty.stringValue = "IDLE_UR";
    break;
  case Direction::down_left:
    animationProperty.stringValue = "IDLE_DL";
    break;
  case Direction::down_right:
    animationProperty.stringValue = "IDLE_DR";
    break;
  }

  ActorPropertyAnimator::PropertyStep axisProperty;
  axisProperty.ease = Ease::linear;

  auto isConveyor = [&layer, &map](sf::Vector2f endTilePos) {
    auto tile = layer.GetTile(int(endTilePos.x), int(endTilePos.y));

    if (!tile) {
      return false;
    }

    auto tileMeta = map.GetTileMeta(tile->gid);

    return tileMeta && tileMeta->type == TileType::conveyor;
  };

  // resolve end position
  switch (direction) {
  case Direction::up_left:
  case Direction::down_right:
    endTilePos.x = std::floor(endTilePos.x);
    axisProperty.property = ActorProperty::x;
    break;
  case Direction::up_right:
  case Direction::down_left:
    axisProperty.property = ActorProperty::y;
    endTilePos.y = std::floor(endTilePos.y);
    break;
  }

  auto unprojectedDirection = Orthographic(direction);
  auto walkVector = UnitVector(unprojectedDirection);

  endTilePos += walkVector;

  bool nextTileIsConveyor = isConveyor(endTilePos);
  if (nextTileIsConveyor) {
    endTilePos += walkVector / 2.0f;
  }

  // fixing overshooting
  switch (direction) {
  case Direction::up_left:
    endTilePos.x = std::nextafter(endTilePos.x + 1.0f, -INFINITY);
    break;
  case Direction::up_right:
    endTilePos.y = std::nextafter(endTilePos.y + 1.0f, -INFINITY);
    break;
  }

  auto endPos = map.TileToWorld(endTilePos);

  switch (axisProperty.property) {
  case ActorProperty::x:
    axisProperty.value = endPos.x;
    tileDistance = std::abs(endTilePos.x - startTilePos.x);
    break;
  case ActorProperty::y:
    axisProperty.value = endPos.y;
    tileDistance = std::abs(endTilePos.y - startTilePos.y);
    break;
  }

  propertyAnimator.Reset();

  auto speed = tileMeta.customProperties.GetPropertyFloat("speed");

  if (speed == 0.0f) {
    speed = DEFAULT_CONVEYOR_SPEED;
  }

  auto duration = tileDistance / speed;

  ActorPropertyAnimator::PropertyStep sfxProperty;
  sfxProperty.property = ActorProperty::sound_effect_loop;
  sfxProperty.stringValue = scene.GetPath(tileMeta.customProperties.GetProperty("sound effect")).u8string();

  ActorPropertyAnimator::KeyFrame startKeyframe;
  startKeyframe.propertySteps.push_back(animationProperty);
  startKeyframe.propertySteps.push_back(sfxProperty);
  propertyAnimator.AddKeyFrame(startKeyframe);

  ActorPropertyAnimator::KeyFrame endKeyframe;
  endKeyframe.propertySteps.push_back(axisProperty);
  endKeyframe.duration = duration;
  propertyAnimator.AddKeyFrame(endKeyframe);

  if (!nextTileIsConveyor) {
    ActorPropertyAnimator::KeyFrame waitKeyFrame;
    // reuse last property to simulate idle
    waitKeyFrame.propertySteps.push_back(axisProperty);
    waitKeyFrame.duration = 0.25;
    propertyAnimator.AddKeyFrame(waitKeyFrame);
  }

  propertyAnimator.UseKeyFrames(actor);
}

void Overworld::TileBehaviors::HandleIce(SceneBase& scene, Actor& actor, ActorPropertyAnimator& propertyAnimator, TileMeta& tileMeta, Tile& tile) {
  auto& map = scene.GetMap();
  auto layerIndex = actor.GetLayer();
  auto& layer = map.GetLayer(layerIndex);

  // get the slide speed
  auto speed = tileMeta.customProperties.GetPropertyFloat("speed");

  if (speed == 0.0f) {
    speed = DEFAULT_ICE_SPEED;
  }

  // start animation with the ice sound effect
  ActorPropertyAnimator::PropertyStep sfxProperty;
  sfxProperty.property = ActorProperty::sound_effect;
  sfxProperty.stringValue = scene.GetPath(tileMeta.customProperties.GetProperty("sound effect")).u8string();

  // set the animation speed to 0 so the player appears to slip
  ActorPropertyAnimator::PropertyStep animationSpeedProperty(ActorProperty::animation_speed, 0.0f);

  ActorPropertyAnimator::KeyFrame startKeyframe;
  startKeyframe.propertySteps.push_back(animationSpeedProperty);
  startKeyframe.propertySteps.push_back(sfxProperty);
  propertyAnimator.AddKeyFrame(startKeyframe);

  // resolve edge positions
  auto isNotIce = [&layer, &map](sf::Vector2i tilePos, sf::Vector2f rayPos, sf::Vector2f ray) {
    auto tile = layer.GetTile(tilePos.x, tilePos.y);

    if (!tile) {
      return true;
    }

    auto tileMeta = map.GetTileMeta(tile->gid);

    return !(tileMeta && tileMeta->type == TileType::ice);
  };

  auto startTilePos = map.WorldToTileSpace(actor.getPosition());
  auto direction = actor.GetHeading();
  auto ray = Round(UnitVector(direction));
  auto edgeTilePos = CastRay(startTilePos, ray, isNotIce) + map.WorldToTileSpace(ray); // overshoot to get off the ice

  auto edgePos = map.TileToWorld(edgeTilePos);
  ActorPropertyAnimator::PropertyStep xProperty(ActorProperty::x, edgePos.x, Ease::linear);
  ActorPropertyAnimator::PropertyStep yProperty(ActorProperty::y, edgePos.y, Ease::linear);

  ActorPropertyAnimator::KeyFrame edgeKeyframe;
  edgeKeyframe.propertySteps.push_back(xProperty);
  edgeKeyframe.propertySteps.push_back(yProperty);
  edgeKeyframe.duration = Distance(startTilePos, edgeTilePos) / speed;
  propertyAnimator.AddKeyFrame(edgeKeyframe);

  // not on a walkable tile, bounce off the wall and keep going
  if (!map.CanMoveTo(edgeTilePos.x, edgeTilePos.y, (float)layerIndex, layerIndex)) {
    // rewind a bit to get back on the ice
    auto newRayStart = edgeTilePos - map.WorldToTileSpace(ray * 2.f);

    // cast two rays
    auto xEdgeTilePos = CastRay(newRayStart, { ray.x, 0.0f }, isNotIce);
    auto yEdgeTilePos = CastRay(newRayStart, { 0.0f, ray.y }, isNotIce);

    // pick the ray that moved us the furthest
    auto newEdgeTilePos =
      SquaredDistance(newRayStart, xEdgeTilePos) > SquaredDistance(newRayStart, yEdgeTilePos)
        ? xEdgeTilePos
        : yEdgeTilePos;

    // create keyframe
    auto edgePos = map.TileToWorld(newEdgeTilePos) + ray; // overshoot to get off the ice
    ActorPropertyAnimator::PropertyStep xProperty(ActorProperty::x, edgePos.x, Ease::linear);
    ActorPropertyAnimator::PropertyStep yProperty(ActorProperty::y, edgePos.y, Ease::linear);

    ActorPropertyAnimator::KeyFrame edgeKeyframe;
    edgeKeyframe.propertySteps.push_back(xProperty);
    edgeKeyframe.propertySteps.push_back(yProperty);
    edgeKeyframe.duration = Distance(edgeTilePos, newEdgeTilePos) / speed;
    propertyAnimator.AddKeyFrame(edgeKeyframe);
  }

  propertyAnimator.UseKeyFrames(actor);
}

void Overworld::TileBehaviors::HandleTreadmill(SceneBase& scene, Actor& actor, ActorPropertyAnimator& propertyAnimator, TileMeta& tileMeta, Tile& tile) {
  auto& map = scene.GetMap();
  auto moveDirection = Orthographic(tileMeta.direction);
  auto speed = tileMeta.customProperties.GetPropertyFloat("speed");

  if (speed == 0.0f) {
    speed = DEFAULT_TREADMILL_SPEED;
  }
  // divide by fps to get movement for the frame
  auto tileMovement = UnitVector(moveDirection) * speed / 60.0f;
  auto movement = map.TileToWorld(tileMovement);

  auto pos = actor.getPosition() + movement;

  actor.setPosition(pos);
}
