#include "bnCardAction.h"
#include "bnArtifact.h"

class Field;

class TomahawkSwingCardAction : public CardAction {
public:
  TomahawkSwingCardAction(Character& owner);
  ~TomahawkSwingCardAction();

  void Execute() override;
  void EndAction() override;
  void OnAnimationEnd() override;
};

class TomahawkEffect : public Artifact {
public:
  TomahawkEffect(Field* field);
  ~TomahawkEffect();
  void OnUpdate(float elapsed) override;
  void OnDelete() override;
};