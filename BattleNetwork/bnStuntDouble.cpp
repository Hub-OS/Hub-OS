#include "bnStuntDouble.h"
#include "bnAnimationComponent.h"
#include "bnPaletteSwap.h"
StuntDouble::StuntDouble(Character& ref)
{
  // Copy attributes & stats
  setTexture(ref.getTexture());
  setScale(ref.getScale());
  SetTeam(ref.GetTeam());
  SetFacing(ref.GetFacing());
  SetHealth(ref.GetHealth());

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
}
