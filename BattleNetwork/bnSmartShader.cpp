#include "bnSmartShader.h"

  SmartShader::SmartShader() {
    ref = nullptr;
  }

  SmartShader::SmartShader(const SmartShader& copy) {
    funiforms = copy.funiforms;
    iuniforms = copy.iuniforms;
    vfuniforms = copy.vfuniforms;
    ref = copy.ref;
  }

  SmartShader::~SmartShader() {
    iuniforms.clear();
    funiforms.clear();
    ref = nullptr;
  }

  SmartShader::SmartShader(const sf::Shader& rhs) {
    ref = &const_cast<sf::Shader&>(rhs);
  }

 SmartShader& SmartShader::operator=(const sf::Shader& rhs) {
   ref = &const_cast<sf::Shader&>(rhs);
   return *this;
  }

 SmartShader& SmartShader::operator=(const sf::Shader* rhs) {
   ref = const_cast<sf::Shader*>(rhs);
   return *this;
 }

 sf::Shader* SmartShader::Get() {
   ApplyUniforms();
   return ref;
 }

  void SmartShader::ApplyUniforms() {
    iiter iIter = iuniforms.begin();
    fiter fIter = funiforms.begin();
    vfiter vfIter = vfuniforms.begin();
    coliter colIter = coluniforms.begin();

    for (; iIter != iuniforms.end(); iIter++) {
      ref->setUniform(iIter->first, iIter->second);
    }

    for (; fIter != funiforms.end(); fIter++) {
      ref->setUniform(fIter->first, fIter->second);
    }

    for (; vfIter != vfuniforms.end(); vfIter++) {
      ref->setUniform(vfIter->first, vfIter->second);
    }

    for (; colIter != coluniforms.end(); colIter++) {
      sf::Color col = colIter->second;
      ref->setUniform(colIter->first, 
        sf::Glsl::Vec4{ 
          col.r/255.f,
          col.g/255.f, 
          col.b/255.f, 
          col.a/255.f 
        }
      );
    }
  }

  void SmartShader::ResetUniforms() {
    if (!ref) {
      return;
    }

    iiter iIter = iuniforms.begin();
    fiter fIter = funiforms.begin();
    vfiter vfIter = vfuniforms.begin();
    coliter colIter = coluniforms.begin();

    for (; iIter != iuniforms.end(); iIter++) {
      ref->setUniform(iIter->first, 0);
    }

    for (; fIter != funiforms.end(); fIter++) {
      ref->setUniform(fIter->first, 0.f);
    }

    for (; vfIter != vfuniforms.end(); vfIter++) {
      ref->setUniform(vfIter->first, 0.f);
    }

    for (; colIter != coluniforms.end(); colIter++) {
      ref->setUniform(colIter->first, 0.f);
    }

    iuniforms.clear();
    funiforms.clear();
    vfuniforms.clear();
    coluniforms.clear();
  }

  void SmartShader::SetUniform(std::string uniform, float fvalue) {
    funiforms[uniform] = fvalue;
  }

  void SmartShader::SetUniform(std::string uniform, int ivalue) {
    iuniforms[uniform] = ivalue;
  }

  void SmartShader::SetUniform(std::string uniform, const sf::Vector2f& vfvalue) {
    vfuniforms[uniform] = vfvalue;
  }

  void SmartShader::SetUniform(std::string uniform, const sf::Color& colvalue)
  {
    coluniforms[uniform] = colvalue;
  }

  void SmartShader::Reset() {
    ResetUniforms();
    ref = nullptr;
  }
