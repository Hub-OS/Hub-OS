#pragma once
#include <SFML/Graphics.hpp>
#include <map>

class SmartShader
{
  friend class Engine;
private:
  sf::Shader* ref;
  std::map<std::string, int>    iuniforms;
  std::map<std::string, float>  funiforms;
  std::map<std::string, double> duniforms;
  std::map<std::string, sf::Vector2f> vfuniforms;
  // TODO: add support for other unform types as needed

  typedef std::map<std::string, int>::iterator iiter;
  typedef std::map<std::string, float>::iterator fiter;
  typedef std::map<std::string, sf::Vector2f>::iterator vfiter;

  void ApplyUniforms();
  void ResetUniforms();

public:
  SmartShader();
  SmartShader(const SmartShader&);
  ~SmartShader();
  SmartShader(const sf::Shader& rhs);
  SmartShader& operator=(const sf::Shader& rhs);
  SmartShader& operator=(const sf::Shader* rhs);
  void SetUniform(std::string uniform, float fvalue);
  void SetUniform(std::string uniform, int ivalue);
  void SetUniform(std::string uniform, const sf::Vector2f& vfvalue);
  void Reset();
  sf::Shader* Get();
};

