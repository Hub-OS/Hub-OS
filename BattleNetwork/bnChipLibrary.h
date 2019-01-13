#pragma once
#include "bnChip.h"
#include <list>

using std::list;

class ChipLibrary {
public:
  typedef list<Chip>::iterator Iter;

  ChipLibrary();
  ~ChipLibrary();
  static ChipLibrary & GetInstance();

  Iter Begin();
  Iter End();
  const unsigned GetSize() const;

  static const  Element GetElementFromStr(std::string);

  void AddChip(Chip chip);
  const bool IsChipValid(const Chip& chip);
  const Chip FindChip(const std::string name, const char code);

protected:
  void LoadLibrary();

private:
  list<Chip> library;
};

#define CHIPLIB ChipLibrary::GetInstance()

