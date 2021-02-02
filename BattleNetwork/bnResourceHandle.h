#pragma once
#include <memory>
#include <assert.h>

class TextureResourceManager;
class AudioResourceManager;
class ShaderResourceManager;
class ScriptResourceManager;
//class FileResourceManager;

class ResourceHandle {
  friend class Game;

private:
  static TextureResourceManager* textures;
  static AudioResourceManager* audio;
  static ShaderResourceManager* shaders;
  static ScriptResourceManager* scripts;
  //static FileResourceManager* files;

public:

  //
  // standard functions
  //

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

  ScriptResourceManager& Scripts() {
    assert(scripts != nullptr && "script resource manager was nullptr!");
    return *scripts;
  }

  //
  // const-qualified functions
  //

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

  ScriptResourceManager& Scripts() const {
    assert(scripts != nullptr && "script resource manager was nullptr!");
    return *scripts;
  }
};