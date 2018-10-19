#pragma once

class Activity {
public:
  virtual void OnStart() = 0;
  virtual void OnUpdate(double _elapsed) = 0;
  virtual void OnLeave() = 0;
  virtual void OnResume() = 0;
  virtual void OnDraw(sf::RenderTexture& surface) = 0;
  virtual void OnEnd() = 0;
  virtual ~Activity() { ; }
};