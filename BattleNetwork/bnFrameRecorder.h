#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

class FrameRecorder {
private:
  const sf::Window &window;

  std::thread consumerThread;

  bool stopped = false;
  std::deque<sf::Texture> usedTextures;
  std::mutex usedTexturesLock;
  std::condition_variable usedTexturesCv;

  std::vector<sf::Texture> freeTextures;
  std::mutex freeTexturesLock;
  std::condition_variable freeTexturesCv;

  int totalFramesProcessed = 0;

  void flush();

public:
  explicit FrameRecorder(const sf::Window &window);
  ~FrameRecorder();

  void capture();
};
