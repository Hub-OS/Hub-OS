#include "bnStuntDouble.h"
#include "bnAnimationComponent.h"
#include "bnPaletteSwap.h"
StuntDouble::StuntDouble(Character& ref) : ref(ref)
{
  // Copy attributes & stats
  setTexture(ref.getTexture());
  setScale(ref.getScale());
  SetTeam(ref.GetTeam());
  SetFacing(ref.GetFacing());
  SetHealth(ref.GetHealth());
  defaultColor = ref.getColor();

  // copy nodes
  for (auto node : ref.GetChildNodes()) {
    AddNode(node);
  }

  // add common components and copy
  auto* anim = CreateComponent<AnimationComponent>(this);
  auto* pswap = CreateComponent<PaletteSwap>(this);

  // palette swap if applicable
  if (auto otherPswap = ref.GetFirstComponent<PaletteSwap>()) {
    pswap->CopyFrom(*otherPswap);
    pswap->Apply();
  }
  else {
    pswap->Enable(false);
  }

  // animation component if applicable
  if (auto otherAnim = ref.GetFirstComponent<AnimationComponent>()) {
    anim->CopyFrom(*otherAnim);

    // sync items in the animation if applicable
    for (auto&& item : otherAnim->GetSyncItems()) {
      anim->AddToSyncList(item);
    }

    // refresh anim and sprite
    anim->SetAnimation(otherAnim->GetAnimationString());
    anim->Refresh();
  }
}

StuntDouble::~StuntDouble()
{

}

void StuntDouble::OnDelete()
{
}

void StuntDouble::OnUpdate(double elapsed)
{
  // If, and only if, an action changes the user's colors (e.g. partially translucent)
  // copy the user's new colors
  // if (ref.getColor() != defaultColor) {
    setColor(ref.getColor());
  //}
}

bool StuntDouble::CanMoveTo(Battle::Tile*)
{
  return true;
}
