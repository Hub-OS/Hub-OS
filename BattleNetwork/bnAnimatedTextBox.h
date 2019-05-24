#pragma once
#include "bnTextBox.h"
#include "bnAnimation.h"
#include <Swoosh/Ease.h>

/**
 * @class AnimatedTextBox
 * @author mav
 * @date 13/05/19
 * @brief Animates a mmbn textbox with a mugshot that animates with text
 * 
 * @warning WIP utilities to ask questions but is not working as expected
 * 
 * You can enqueue messages and have them show up in dialogue.
 * This makes for easy story telling.
 * You must manually dequeue messages from the text box.
 * This is intentional bc multiple choice questions can take out entire queues of messages.
 * 
 * e.g. Tutorial textbox can dequeue and enqueue the last messages until user says "Dont repeat"
 *      then it can say the last few messages left in queue.
 */
class AnimatedTextBox : public sf::Drawable, public sf::Transformable {
public:
  /**
   * @class Message
   * @author mav
   * @date 13/05/19
   * @brief Message object takes a string.
   * 
   * Intended to be expanded upon to handle multiple choice questions 
   * and even input.
   */
  class Message {
  protected:
    std::string message;
    bool isQuestion;
    bool yes; /*!< Flag for if yes was selected */
    std::function<void()> onYes; /*!< Callback when user presses yes */
    std::function<void()> onNo; /*!< Callback when user presses no */

  public:
    const std::string GetMessage() { return message; }
    const bool IsQuestion() { return isQuestion; }
    const bool SelectYes() { if (!isQuestion) return false; else yes = true; return true; }
    const bool SelectNo() { if (!isQuestion) return false; else yes = false; return true; }

    const bool IsYes() const { return yes; }
    void ExecuteSelection() {
      if (yes) {  onYes();  }
      else {
        { onNo(); }
      }
    }

    Message(std::string message) : message(message) {
      isQuestion = yes = false;
      //onNo = std::function<void()>();
      //onYes = std::function<void()>();
    }

    Message(const Message& rhs) {
      message = rhs.message;
      isQuestion = rhs.isQuestion;
      yes = rhs.yes;
      onYes = rhs.onYes;
      onNo = rhs.onNo;
    }

    virtual ~Message() { ;  }
  };

  /**
   * @class Question
   * @author mav
   * @date 13/05/19
   * @brief Initializes yes/no message objects
   * 
   * @warning honestly this design could be changed to do without the hackery and should see changes
   */
  class Question : public Message {
  public:
    Question(std::string message, std::function<void()> onYes = std::function<void()>(), std::function<void()> onNo = std::function<void()>()) : Message(message) {
      isQuestion = true; 
      this->onNo = onNo;
      this->onYes = onYes;
      this->message += std::string("\\nYes      No");
    }
  };

private:
  mutable sf::Sprite frame; /*!< Size is calculated from the frame sprite */ 
  mutable sf::Sprite nextCursor; /*!< Green cursor at bottom-right */
  mutable sf::Sprite selectCursor; /*!< Used for making selections */

  mutable Animation mugAnimator; /*!< Animates the mugshot frames */
  bool isPaused; /*!< Pause text flag */
  bool isReady; /*!< Ready to type text flag */
  bool isOpening; /*!< Opening textbox flag */
  bool isClosing; /*!< Closing textbox flag */
  mutable std::vector<sf::Sprite> mugshots; /*!< List of current and next mugshots */
  std::vector<std::string> animPaths; /*!< List of animation paths for the mugshots */
  std::vector<Message*> messages; /*!< Lists of current and next messages */
  Animation animator; /*!< Animator for the textbox */

  sf::IntRect textArea; /*!< The area for text to type in */
  TextBox textBox; /*!< Textbox object types text out for us */

  double totalTime; /*!< elapsed */
  double textSpeed; /*!< desired speed of text */
public:
  /**
   * @brief constructs AnimatedTextBox at given position on screen 
   **/
  AnimatedTextBox(sf::Vector2f pos);
  virtual ~AnimatedTextBox();

  /**
   * @brief Remove message and mugshot from queue
   */
  void DequeMessage();
  
  /**
   * @brief Adds message and mugshot to queue
   * @param speaker mugshot sprite
   * @param animationPath mugshot animations list
   * @param message message object
   */
  void EnqueMessage(sf::Sprite speaker, std::string animationPath, Message* message);
  
  /**
   * @brief Begins closing the textbox
   */
  void Close();
  
  /**
   * @brief Begins opening the textbox
   */
  void Open();

  /**
   * @brief Selects yes if applicable
   * @return true if success, false otherwise
   */
  const bool SelectYes() const;
  
  /**
   * @brief Selects no if applicable
   * @return true if success, false otherwise
   */
  const bool SelectNo() const;
  
  /**
   * @brief Confirms yes/no selection
   * @return true if success, false otherwise
   */
  const bool ConfirmSelection();
  
  /**
   * @brief Continues the text if it didn't fit the line. 
   * 
   * If there are more messages and the current one is done printing, will deque
   * the message and begin the next one
   */
  void Continue();

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
   * @brief Update the animated textbox
   * @param elapsed in seconds
   */
  virtual void Update(float elapsed);
  
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
};
