#pragma once
#include "bnTextBox.h"
#include "bnAnimation.h"
#include "bnMessageInterface.h"
#include "bnResourceHandle.h"
#include "bnSceneNode.h"
#include "bnSpriteProxyNode.h"
#include "bnAudioResourceManager.h"
#include <Swoosh/Ease.h>

/**
 * @class AnimatedTextBox
 * @author mav
 * @date 13/05/19
 * @brief Animators a mmbn textbox with a mugshot that animates with text
 * 
 * You can enqueue messages and have them show up in dialogue.
 * This makes for easy story telling.
 * You must manually dequeue messages from the text box.
 * This is intentional bc multiple choice questions can take out entire queues of messages.
 * 
 * e.g. Tutorial textbox can dequeue and enqueue the last messages until user says "Dont repeat"
 *      then it can say the last few messages left in queue.
 */
class AnimatedTextBox : public SceneNode, public ResourceHandle {
private:
  bool isPaused{}; /*!< Pause text flag */
  bool isReady{}; /*!< Ready to type text flag */
  bool isOpening{}; /*!< Opening textbox flag */
  bool isClosing{}; /*!< Closing textbox flag */
  mutable bool lightenMug{true};
  double totalTime{}; /*!< elapsed */
  double textSpeed{1.0}; /*!< desired speed of text */
  mutable std::vector<sf::Sprite> mugshots; /*!< List of current and next mugshots */
  mutable std::shared_ptr<SpriteProxyNode> lastSpeaker;
  std::vector<Animation> anims; /*!< List of animation paths for the mugshots */
  std::vector<MessageInterface*> messages; /*!< Lists of current and next messages */
  mutable std::shared_ptr<SpriteProxyNode> frame; /*!< Size is calculated from the frame sprite */
  mutable Animation mugAnimator; /*!< Animators the mugshot frames */
  Animation animator; /*!< Animator for the textbox */
  std::shared_ptr<Texture> textureRef; /*!< smart reference to the texture*/
  sf::IntRect textArea; /*!< The area for text to type in */
  std::shared_ptr<TextBox> textBox; /*!< Textbox object types text out for us */

public:
  /**
   * @brief constructs AnimatedTextBox at given position on screen 
   **/
  AnimatedTextBox(const sf::Vector2f& pos);
  virtual ~AnimatedTextBox();

  /**
   * @brief Remove message and mugshot from queue
   */
  void DequeMessage();

  void ClearAllMessages();
  
  /**
   * @brief Adds message and mugshot to queue
   * @param speaker mugshot sprite
   * @param animation mugshot animation
   * @param message message object
   */
  void EnqueMessage(const sf::Sprite& speaker, const Animation& anim, MessageInterface* message);
  
  /**
   * @brief Adds message to queue
   * @param message message object
   * 
   * This variation does not display a talking face
   */
  void EnqueMessage(MessageInterface* message);

  void ReplaceText(std::string text);

  /**
   * @brief Begins closing the textbox
   */
  void Close();
  
  /**
   * @brief Begins opening the textbox
   */
  void Open(const std::function<void()>& onOpen = nullptr);

  /**
   * @brief Query if the textbox is playing
   * @return true if playing, false if paused
   */
  const bool IsPlaying() const;
  
  /**
   * @brief Query if the textbox is fully open
   * @return true if fully open and not animating, false if inbetween or closed
   */
  const bool IsOpen() const;
  
  /**
   * @brief Query if the textbox is fully closed
   * @return true if fully closed and not animating, false if inbetween or open
   */
  const bool IsClosed() const;
  
  /**
   * @brief Query if there are more messages
   * @return true if there are 1 or more messages in queue, false otherwise
   */
  const bool HasMessage();

  /**
   * @brief Query if the message has finished or if there is more text waiting to print
   * @return false if there are 1 or more messages in queue and the textbox isn't finished, true otherwise
   */
  const bool IsEndOfMessage();

  const bool IsEndOfBlock();

  /**
   * @brief Query if the current message has another block
   * @return false if the textbox has more blocks, true otherwise
   */
  bool IsFinalBlock() const;

  /**
   * @brief Will remove the first line and add the next line to ensure all text fits
   */
  void ShowNextLines();

  void ShowPreviousLines();

  void CompleteCurrentBlock();

  /**
  * @brief returns the number of fitting lines in the underlyning textbox object
  */
  const int GetNumberOfFittingLines() const;

  const float GetFrameWidth() const;
  const float GetFrameHeight() const;

  /**
   * @brief Returns the character range of the displayed text
   * @return pair of size_t
   */
  std::pair<size_t, size_t> GetCurrentCharacterRange() const;

  /**
   * @brief Returns the range of the displayed lines (not the range of the characters)
   * @return pair of size_t
   */
  std::pair<size_t, size_t> GetCurrentLineRange() const;

  /**
   * @brief Returns the character range of the completed text
   * @return pair of size_t
   */
  std::pair<size_t, size_t> GetBlockCharacterRange() const;

  const int GetTextboxAreaWidth() const;
  const int GetTextboxAreaHeight() const;

  /**
   * @brief Update the animated textbox
   * @param elapsed in seconds
   */
  virtual void Update(double elapsed);
  
  /**
   * @brief Set desired text speed
   * @param factor Characters Per Second
   */
  void SetTextSpeed(double factor);
  
  /**
   * @brief Draw the textbox, the mugshot, the animated frame, and all cursors
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  void DrawMessage(sf::RenderTarget& target, sf::RenderStates states) const;

  Text MakeTextObject(const std::string& data = std::string());

  void ChangeAppearance(std::shared_ptr<sf::Texture> newTexture, const Animation& newAnimation);
  void ChangeBlipSfx(std::shared_ptr<sf::SoundBuffer> newSfx);

  Font GetFont() const;
  sf::Vector2f GetTextPosition() const;

  void Mute(bool enabled = true);
  void Unmute();
};
