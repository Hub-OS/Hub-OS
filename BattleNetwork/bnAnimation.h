#pragma once
#include <string>
#include <assert.h>
#include <functional>

#include <iostream>

#include "bnAnimate.h"

using std::string;
using std::to_string;

#define ANIMATION_EXTENSION ".animation"

/**
 * @class Animation
 * @author mav
 * @date 14/05/19
 * @see bnAnimate.h
 * @brief Loads a FrameList from an animation file format and animates a sprite
 * 
 * Basically a wrapper around the animator that knows the possible animations.
 * 
 * File format example:
 * 
 * VERSION="1.0"
 * imagePath=C:\User\Bob\Desktop\player.png
 * 
 * animation state="PLAYER_SHOOTING"
 * frame duration="0.05" x="0" y="10" w="70" h="47" originx="21" originy="40"
 * point label="Buster" x="42" y="15"
 * frame duration="0.05" x="72" y="7" w="70" h="47" originx="23" originy="40"
 * point label="Buster" x="42" y="15"
 * frame duration="0.05" x="149" y="11" w="75" h="47" originx="21" originy="39"
 * point label="Buster" x="40" y="14"
 * frame duration="0.05" x="225" y="11" w="77" h="47" originx="20" originy="39"
 * point label="Buster" x="39" y="14"
 * frame duration="0.05" x="302" y="11" w="68" h="47" originx="18" originy="39"
 * point label="Buster" x="38" y="14"
 * 
 * animation state="PLAYER_MOVING"
 * ...
 * 
 * etc.
 */
class Animation {
public:

  /**
   * @brief No frame list is loaded*/
  Animation();
  
  /**
   * @brief Parses file at path and populates FrameList with data
   * @param path relative path from application to file
   */
  Animation(const char* path);
  
  /**
   * @brief Parses file at path and populates FrameList with data
   * @param path relative path from application to file
   */
  Animation(string path);
  ~Animation();

<<<<<<< HEAD
  void Reload();
  void Update(float _elapsed, sf::Sprite& target, double playbackSpeed = 1.0);
  void Refresh(sf::Sprite& target);
  void SetFrame(int frame, sf::Sprite& target);
  void SetAnimation(string state);

  void RemoveCallbacks();

  const std::string GetAnimationString() const;

=======
  /**
   * @brief Reads file at path set by constructor, parses lines, and populates FrameList with data
   */
  void Reload();
  
  /**
   * @brief Apply FrameList to sprite
   * @param _elapsed in seconds to add to progress
   * @param target sprite to apply to
   * @param playbackSpeed virtually simulate speed
   */
  void Update(float _elapsed, sf::Sprite& target, double playbackSpeed = 1.0);
  
  /**
   * @brief Sets progress to 0 and updates sprite. Same as a call to Update(0, target).
   * @param target
   */
  void Refresh(sf::Sprite& target);
  
  /**
   * @brief Manually set a frame to the sprite
   * @param frame Base 1 frame index
   * @param target Sprite to update
   * 
   * If frame is out of bounds for the current animation, ignores
   */
  void SetFrame(int frame, sf::Sprite& target);
  
  /**
   * @brief Sets the current animation from a map of FrameLists
   * @param state animation name is the map key
   */
  void SetAnimation(string state);

  /**
   * @brief Clears the function callbacks in the animator
   */
  void RemoveCallbacks();

  /**
   * @brief Get the current animation state name
   * @return If not animation state is set, returns empty string
   */
  const std::string GetAnimationString() const;

  /**
   * @brief Get the frame list corresponding to this animation state
   * @param animation name of the animation
   * @return FrameList&
   * @warning Make sure this animation exists otherwise could return an empty frame list or throw
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  FrameList& GetFrameList(std::string animation);

  /**
   * @brief Append frame callback
   * @param rhs On struct
   * @return Animation& to chain
   */
  Animation& operator<<(Animate::On rhs);
<<<<<<< HEAD
  Animation& operator<<(char rhs);
  Animation& operator<<(std::string state);

=======
  
  /**
   * @brief Set the animator mode
   * @param rhs byte mode
   * @return Animation& to chain
   */
  Animation& operator<<(char rhs);
  
  /**
   * @brief Set the animation state
   * @param state name to chain
   */
  Animation& operator<<(std::string state);

  /**
   * @brief Set the onFinish callback
   * @param onFinish the function to call when the animation finishes
   * @warning does not return object. This must be the end of the chain.
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void operator<<(std::function<void()> onFinish);

private:
  /**
   * @brief Strips the key-value from a file format
   * @param _key to look for value of
   * @param _line string input
   * @return value as string or empty string
   */
  string ValueOf(string _key, string _line);
protected:
<<<<<<< HEAD
  Animate animator;

  string path;
  string currAnimation;
  float progress;
  std::map<int, sf::Sprite> textures;
  std::map<string, FrameList> animations;
=======
  Animate animator; /*!< Internal animator to delegate most of the work to */
  string path; /*!< Path to the animation file */
  string currAnimation; /*!< Name of the current animation state */
  float progress; /*!< Current progress of animation */
  std::map<string, FrameList> animations; /*!< Dictionary of FrameLists read from file */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};
