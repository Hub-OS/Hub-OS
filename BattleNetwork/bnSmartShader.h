/*! \brief A shader wrapper that intelligently applies itself during draw calls
 * 
 * Currently supports int, float, double, vector2f uniforms
 * 
 * Additional uniforms must be added
 */

#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <map>

class SmartShader
{
  friend class DrawWindow;

private:
  sf::Shader* ref; /*!< Pointer to shader object */
  std::map<std::string, int>    iuniforms; /*!< lookup of integer uniforms */
  std::map<std::string, float>  funiforms; /*!< lookup of float uniforms */
  std::map<std::string, double> duniforms; /*!< lookup of double uniforms */
  std::map<std::string, std::vector<float>> farruniforms; /*! lookup of float arrays */
  std::map<std::string, sf::Vector2f> vfuniforms; /*!< lookup of vector2f uniforms */
  std::map<std::string, sf::Color> coluniforms; /*!< lookup of sf::Color uniforms */
  std::map<std::string, std::shared_ptr<sf::Texture>> texuniforms; /*! lookups of texture uniforms */
  std::map<std::string, sf::Shader::CurrentTextureType> textypeuniforms;

  typedef std::map<std::string, int>::iterator iiter; 
  typedef std::map<std::string, float>::iterator fiter;
  typedef std::map<std::string, double>::iterator diter;
  typedef std::map<std::string, std::vector<float>>::iterator farriter;
  typedef std::map<std::string, sf::Vector2f>::iterator vfiter;
  typedef std::map<std::string, sf::Color>::iterator coliter;
  typedef std::map<std::string, std::shared_ptr<sf::Texture>>::iterator texiter;
  typedef std::map<std::string, sf::Shader::CurrentTextureType>::iterator textypeiter;

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
  SmartShader& operator=(const sf::Shader* rhs);
  
  /**
   * @brief Set a float uniform value
   * @param uniform the name of the uniform
   * @param fvalue
   */
  void SetUniform(std::string uniform, float fvalue);

  /**
   * @brief Set a double uniform value
   * @param uniform the name of the uniform
   * @param dvalue
   */
  void SetUniform(std::string uniform, double dvalue);

  /**
   * @brief Set a float array uniform value
   * @param uniform the name of the uniform
   * @param farrvalue
   */
  void SetUniform(std::string uniform, const std::vector<float>& farr);
  
  /**
   * @brief Set an integer uniform value
   * @param uniform the name of the uniform
   * @param ivalue
   */
  void SetUniform(std::string uniform, int ivalue);
  
  /**
   * @brief Set a vector2f uniform value
   * @param uniform the name of the uniform
   * @param vfvalue
   */
  void SetUniform(std::string uniform, const sf::Vector2f& vfvalue);
  
  /**
   * @brief Set a color uniform value
   * @param uniform the name of the uniform
   * @param colvalue
   */
  void SetUniform(std::string uniform, const sf::Color& colvalue);

  /**
   * @brief Set a texture uniform value
   * @param uniform the name of the uniform
   * @param texvalue
   */
  void SetUniform(std::string uniform, const std::shared_ptr<sf::Texture>& texvalue);

  /**
  * @brief Set a texture type uniform value
  * @param uniform the name of the uniform
  * @param value
  */
  void SetUniform(std::string uniform, const sf::Shader::CurrentTextureType& value);

  /**
   * @brief Sets all pre-existing uniforms to 0, empties the lookups, and frees ref
   */
  void Reset();
  
  /**
   * @brief Fetch the shader object
   * @return sf::Shader*
   */
  sf::Shader* Get();

  /**
   * @brief Lighter than checking if Get() returns nullptr
   * @return true if ref is not nullptr
   */
  bool HasShader();
};

