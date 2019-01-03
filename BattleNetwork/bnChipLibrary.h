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

protected:
  void LoadLibrary();

private:
  list<Chip> library;
  string valueOf(string _key, string _line);
};

#define CHIPLIB ChipLibrary::GetInstance()

