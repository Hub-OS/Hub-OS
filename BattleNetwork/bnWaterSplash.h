#include "bnArtifact.h"
#include "bnAnimation.h"

class WaterSplash : public Artifact {
  Animation splashAnim;

public:
  WaterSplash();
  ~WaterSplash();

  void OnUpdate(double elapsed) override;
  void OnDelete() override {};
};