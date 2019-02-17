#pragma once

#include "Swoosh/Ease.h"

#include <SFML/Graphics.hpp>
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnChipFolder.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnChipDescriptionTextbox.h"
#include "bnCustEmblem.h"
#include "bnSceneNode.h"

class ChipSelectionCust : public SceneNode {
public:
  struct Bucket {
    Chip* data;
    short state;
  };

private:
  // TODO: take mutable attr out
  mutable sf::Sprite custSprite;
  mutable sf::Sprite cursorSmall; // animated
  mutable sf::Sprite cursorBig;   // animated
  mutable sf::Sprite chipLock;
  Animation cursorSmallAnimator;
  Animation cursorBigAnimator;
  mutable LayeredDrawable icon;
  mutable LayeredDrawable chipCard;
  LayeredDrawable chipNoData;
  LayeredDrawable chipSendData;
  mutable LayeredDrawable element;
  sf::Shader& greyscale;
  sf::Font* labelFont;
  sf::Font* codeFont;
  mutable sf::Text smCodeLabel;
  mutable sf::Text label;
  mutable CustEmblem emblem;

  int chipCount;
  int selectCount;
  int chipCap;
  mutable int cursorPos;
  mutable int cursorRow;
  bool areChipsReady;
  bool isInView;
  int perTurn;
  ChipFolder* folder;
  Chip** selectedChips;
  Bucket* queue;
  Bucket** selectQueue;
  ChipDescriptionTextbox chipDescriptionTextbox;

  double frameElapsed;

public:
  ChipSelectionCust(ChipFolder* _folder, int, int perTurn = 5);
  ~ChipSelectionCust();

  // GUI ops
  bool OpenChipDescription();
  bool ContinueChipDescription();
  bool FastForwardChipDescription(double factor);
  bool CloseChipDescription();
  bool ChipDescriptionYes();
  bool ChipDescriptionNo();
  bool CursorUp();
  bool CursorDown();
  bool CursorRight();
  bool CursorLeft();
  bool CursorAction();
  bool CursorCancel();

  bool IsOutOfView();
  const bool IsInView();
  bool IsChipDescriptionTextBoxOpen();
  void Move(sf::Vector2f delta);
  const sf::Vector2f GetOffset() const { return custSprite.getPosition() - sf::Vector2f(-custSprite.getTextureRect().width*2.f, 0.f); } // TODO: Get rid. See BattleScene.cpp line 241
  
  void draw(sf::RenderTarget& target, sf::RenderStates states) const;
  void Update(float elapsed);

  bool ChipDescriptionConfirmQuestion();

  // Chip ops
  void GetNextChips();
  Chip** GetChips();
  void ClearChips();
  const int GetChipCount();
  void ResetState();
  bool AreChipsReady();
};

