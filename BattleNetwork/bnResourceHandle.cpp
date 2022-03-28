#include "bnResourceHandle.h"

TextureResourceManager* ResourceHandle::textures{ nullptr };
AudioResourceManager*   ResourceHandle::audio   { nullptr };
ShaderResourceManager*  ResourceHandle::shaders { nullptr };

#ifdef ONB_MOD_SUPPORT
ScriptResourceManager* ResourceHandle::scripts { nullptr };
#endif