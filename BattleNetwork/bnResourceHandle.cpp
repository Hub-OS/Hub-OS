#include "bnResourceHandle.h"

TextureResourceManager* ResourceHandle::textures{ nullptr };
AudioResourceManager*   ResourceHandle::audio   { nullptr };
ShaderResourceManager*  ResourceHandle::shaders { nullptr };

#ifdef BN_MOD_SUPPORT
ScriptResourceManager* ResourceHandle::scripts { nullptr };
#endif