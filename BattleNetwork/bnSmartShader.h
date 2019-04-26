/*! \brief A shader wrapper that intelligently applies itself during draw calls
 * 
 * Currently supports int, float, double, vector2f uniforms
 * 
 * Additional uniforms must be added
 */

#pragma once
#include <SFML/Graphics.hpp>
#include <map>

class SmartShader
{
  friend class Engine;
private:
  sf::Shader* ref; /*!< Pointer to shader object */
  std::map<std::string, int>    iuniforms; /*!< lookup of integer uniforms */
  std::map<std::string, float>  funiforms; /*!< lookup of float uniforms */
  std::map<std::string, double> duniforms; /*!< lookup of double uniforms */
  std::map<std::string, sf::Vector2f> vfuniforms; /*!< lookup of vector2f uniforms */

  typedef std::map<std::string, int>::iterator iiter; 
  typedef std::map<std::string, float>::iterator fiter;
  typedef std::map<std::string, sf::Vector2f>::iterator vfiter;

  /**
   * @brief Applies all registered uniform values before drawing
   */
  void ApplyUniforms();
  
  /**
   * @brief Clears the shader object of all uniform values
   */
  void ResetUniforms();

public:
  /**
   * @brief Constructs a smart shader with pointer to sf::Shader ref set to nullptr
   */
  SmartShader();
  
  /**
   * @brief Constructs a smart shader from another smart shader
   */
  SmartShader(const SmartShader&);
  
  /**
   * @brief Frees the reference to the shader object and empties the uniform dictionaries
   */
  ~SmartShader();
  
  /**
   * @brief Assigns shader object ref to rhs
   * @param rhs shader object to assign itself to
   */
  SmartShader(const sf::Shader& rhs);
  
  /**
   * @brief Assignment ops assigns ref to a shader object rhs
   * @param rhs
   */
  SmartShader& operator=(const sf::Shader& rhs);
  
  /**
   * @brief Assignment ops assigns ref to a shader object rhs
   * @param rhs
   */
  SmartShader& operator=(const sf::Shader* rhs
  
  /**
   * @brief Set a float uniform value
   * @param uniform the name of the uniform
   * @param fvalue
   */
  void SetUniform(std::string uniform, float fvalue);
  
  /**
   * @brief Set an integer uniform value
   * @param uniform the name of the uniform
   * @param ivalue
   */
  void SetUniform(std::string uniform, int ivalue);
  
  /**
   * @brief Set a vector2f uniform values
   * @param uniform the name of the uniform
   * @param vfvalue
   */
  void SetUniform(std::string uniform, const sf::Vector2f& vfvalue);
  
  /**
   * @brief Sets all pre-existing uniforms to 0, empties the lookups, and frees ref
   */
  void Reset();
  
  /**
   * @brief Fetch the shader object
   * @return sf::Shader*
   */
  sf::Shader* Get();
};

