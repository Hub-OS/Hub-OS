#pragma once
#include <memory>
#include <assert.h>

class TextureResourceManager;
class AudioResourceManager;
class FileResourceManager;
class ShaderResourceManager;
class InputMananger;

class ResourceHandle {
  friend class Game;

private:
  static TextureResourceManager* textures;
  static AudioResourceManager* audio;
  static ShaderResourceManager* shaders;
  //static FileResourceMananger* files;
  //static InputManager* input;

public:
  TextureResourceManager& Textures() { 
    assert(textures != nullptr && "texture resource manager was nullptr!");  
    return *textures; 
  }
  AudioResourceManager& Audio() {
    assert(audio != nullptr && "audio resource manager was nullptr!");
    return *audio; 
  }

  ShaderResourceManager& Shaders() { 
    assert(shaders != nullptr && "shader resource manager was nullptr!");
    return *shaders; 
  }

  // const-qualified functions

  TextureResourceManager& Textures() const {
    assert(textures != nullptr && "texture resource manager was nullptr!");
    return *textures;
  }
  AudioResourceManager& Audio() const {
    assert(audio != nullptr && "audio resource manager was nullptr!");
    return *audio;
  }

  ShaderResourceManager& Shaders() const {
    assert(shaders != nullptr && "shader resource manager was nullptr!");
    return *shaders;
  }
};

TextureResourceManager* ResourceHandle::textures = nullptr;
AudioResourceManager  * ResourceHandle::audio    = nullptr;
ShaderResourceManager * ResourceHandle::shaders  = nullptr;