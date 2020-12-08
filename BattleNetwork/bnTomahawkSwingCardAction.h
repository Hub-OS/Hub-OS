#include "bnCardAction.h"
#include "bnArtifact.h"

class Field;

class TomahawkSwingCardAction : public CardAction {
  int damage{};

public:
  TomahawkSwingCardAction(Character& owner, int damage);
  ~TomahawkSwingCardAction();

  void OnExecute() override;
  void OnEndAction() override;
  void OnAnimationEnd() override;
};

class TomahawkEffect : public Artifact {
public:
  TomahawkEffect(Field* field);
  ~TomahawkEffect();
  void OnUpdate(float elapsed) override;
  void OnDelete() override;
};