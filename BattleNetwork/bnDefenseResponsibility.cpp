#include "bnDefenseResponsibility.h"

const bool DefenseResponsibility::Next(Spell *in) {
  if (next) {
    return next->Check(in);
  }

  return true;
}

DefenseResponsibility::DefenseResponsibility() {
  next = nullptr;
}

DefenseResponsibility::~DefenseResponsibility() { }

void DefenseResponsibility::Add(DefenseResponsibility* in) {
  if (next) {
    next->Add(in);
  }
  else {
    next = in;
  }
}