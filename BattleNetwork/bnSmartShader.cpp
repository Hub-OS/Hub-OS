#include "bnSmartShader.h"

  sf::Shader* ref;
  std::map<std::string, int>    iuniforms; std::map<std::string, int>::iterator iiter;
  std::map<std::string, float>  funiforms; std::map<std::string, float>::iterator fiter;
  // NOTE: add support for other unform types as needed

  SmartShader::SmartShader() {
    ref = nullptr;
  }

  SmartShader::SmartShader(const SmartShader& copy) {
    funiforms = copy.funiforms;
    iuniforms = copy.iuniforms;
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
   return ref;
 }

  void SmartShader::ApplyUniforms() {
    iiter = iuniforms.begin();
    fiter = funiforms.begin();

    for (; iiter != iuniforms.end(); iiter++) {
      ref->setUniform(iiter->first, iiter->second);
    }

    for (; fiter != funiforms.end(); fiter++) {
      ref->setUniform(fiter->first, fiter->second);
    }
  }

  void SmartShader::ResetUniforms() {
    if (!ref) {
      return;
    }

    iiter = iuniforms.begin();
    fiter = funiforms.begin();

    for (; iiter != iuniforms.end(); iiter++) {
      ref->setUniform(iiter->first, 0);
    }

    for (; fiter != funiforms.end(); fiter++) {
      ref->setUniform(fiter->first, 0.f);
    }

    iuniforms.clear();
    funiforms.clear();
  }

  void SmartShader::SetUniform(std::string uniform, float fvalue) {
    /*if (ref->getUniformLocation(uniform) == -1) {
      throw new std::exception("Shader program has no uniform named " + uniform);
    }*/
    funiforms.emplace(uniform, fvalue);
  }

  void SmartShader::SetUniform(std::string uniform, int ivalue) {
    /*if (ref->getUniformLocation(uniform) == -1) {
      throw new std::exception("Shader program has no uniform named " + uniform);
    }*/
    iuniforms.emplace(uniform, ivalue);
  }

  void SmartShader::Reset() {
    this->ResetUniforms();
    this->ref = nullptr;
  }
