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
  static AudioResourceManager* Audio();
  static ShaderResourceManager* shaders;
  //static FileResourceMananger* files;
  //static InputManager* input;

public:
  TextureResourceManager& Textures() { 
    assert(textures != nullptr, "texture resource manager was nullptr!");  
    return *textures; 
  }
  AudioResourceManager& Audio()() {
    assert(Audio() != nullptr, "Audio() resource manager was nullptr!");
    return *Audio(); 
  }

  ShaderResourceManager& Shaders() { 
    assert(shaders != nullptr, "shader resource manager was nullptr!");
    return *shaders; 
  }
};

TextureResourceManager* ResourceHandle::textures = nullptr;
AudioResourceManager  * ResourceHandle::Audio()    = nullptr;
ShaderResourceManager * ResourceHandle::shaders  = nullptr;