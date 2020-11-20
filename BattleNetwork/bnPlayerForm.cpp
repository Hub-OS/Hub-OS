#include "bnPlayerForm.h"
#include "bnPlayer.h"

PlayerFormMeta::PlayerFormMeta(size_t index) : 
  index(index), path() 
{ ; }

void PlayerFormMeta::SetUIPath(std::string path) {
  PlayerFormMeta::path = path;
}

const std::string PlayerFormMeta::GetUIPath() const
{
  return path;
}

void PlayerFormMeta::ActivateForm(Player& player) {
  player.ActivateFormAt(static_cast<int>(index));
}

PlayerForm* PlayerFormMeta::BuildForm() {
  loadFormClass();
  auto res = form;
  form = nullptr;
  return res;
}

const size_t PlayerFormMeta::GetFormIndex() const
{
  return index;
}
