#pragma once
#include "bnChip.h"

class ChipUsePublisher;
class Character;

class ChipUseListener {
public:
  ChipUseListener() = default;
  ~ChipUseListener() = default;

  ChipUseListener(const ChipUseListener& rhs) = delete;
  ChipUseListener(ChipUseListener&& rhs) = delete;

  virtual void OnChipUse(Chip& chip, Character& user) = 0;
  void Subscribe(ChipUsePublisher& publisher);
};