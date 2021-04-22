#include "bnCardAction.h"
#include "bnArtifact.h"

class TomahawkSwingCardAction : public CardAction {
  int damage{};

public:
  TomahawkSwingCardAction(Character& actor, int damage);
  ~TomahawkSwingCardAction();

  void OnExecute(Character* user) override;
  void OnEndAction() override;
  void OnAnimationEnd() override;
};

class TomahawkEffect : public Artifact {
public:
  TomahawkEffect();
  ~TomahawkEffect();
  void OnUpdate(double elapsed) override;
  void OnDelete() override;
};