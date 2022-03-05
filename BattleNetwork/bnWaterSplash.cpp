#include "bnWaterSplash.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"

WaterSplash::WaterSplash()
{
  splashAnim = Animation("resources/tiles/splash.animation");
  splashAnim << "SPLASH"  << [this]() {
    this->Erase();
  };;
  
  setScale(2.f, 2.f);
  SetLayer(-1);

  setTexture(Textures().LoadFromFile("resources/tiles/splash.png"));
}

WaterSplash::~WaterSplash()
{
}

void WaterSplash::OnUpdate(double elapsed)
{
  splashAnim.Update(elapsed, getSprite());
}
