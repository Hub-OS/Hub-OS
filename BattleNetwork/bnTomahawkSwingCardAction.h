#include "bnCardAction.h"
#include "bnArtifact.h"

class TomahawkSwingCardAction : public CardAction {
  int damage{};

public:
  TomahawkSwingCardAction(std::shared_ptr<Character> actor, int damage);
  ~TomahawkSwingCardAction();

  void OnExecute(std::shared_ptr<Character> user) override;
  void OnActionEnd() override;
  void OnAnimationEnd() override;
};

class TomahawkEffect : public Artifact {
public:
  TomahawkEffect();
  ~TomahawkEffect();
  void OnUpdate(double elapsed) override;
  void OnDelete() override;
};