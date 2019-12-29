#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;

// TODO: Redesign ChipAction!
class SwordChipAction : public ChipAction {
protected:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  SpriteSceneNode* hiltAttachment;
  Animation attachmentAnim,hiltAttachmentAnim;
  int damage;
  Element element;
public:
  SwordChipAction(Character* owner, int damage);
  virtual ~SwordChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
  virtual void OnSpawnHitbox();
  void SetElement(Element elem);
  const Element GetElement() const;
};

