#ifdef BN_MOD_SUPPORT
#include "bnUserTypeAnimation.h"
#include "../bnScriptResourceManager.h"
#include "../bnSpriteProxyNode.h"
#include "../bnSolHelpers.h"

void DefineAnimationUserType(sol::state& state, sol::table& engine_namespace) {
  engine_namespace.new_usertype<AnimationWrapper>("Animation",
    sol::factories(
      [] (const std::string& path) {
        auto animation = std::make_shared<Animation>(std::filesystem::u8path(path));

        auto animationWrapper = AnimationWrapper(animation, *animation);
        animationWrapper.OwnParent();

        return animationWrapper;
      },
      [] (AnimationWrapper& original) {
        auto animation = std::make_shared<Animation>(original.Unwrap());

        auto animationWrapper = AnimationWrapper(animation, *animation);
        animationWrapper.OwnParent();

        return animationWrapper;
      }
    ),
    sol::meta_function::index, []( sol::table table, const std::string key ) {
      ScriptResourceManager::PrintInvalidAccessMessage( table, "Animation", key );
    },
    sol::meta_function::new_index, []( sol::table table, const std::string key, sol::object obj ) {
      ScriptResourceManager::PrintInvalidAssignMessage( table, "Animation", key );
    },
    "load", [](AnimationWrapper& animation, const std::string& path) {
      animation.Unwrap().Load(std::filesystem::u8path(path));
    },
    "update", [](AnimationWrapper& animation, double elapsed, WeakWrapper<SpriteProxyNode>& target) {
      animation.Unwrap().Update(elapsed, target.Unwrap()->getSprite());
    },
    "set_playback_speed", [](AnimationWrapper& animation, double speed) {
      animation.Unwrap().SetPlaybackSpeed(speed);
    },
    "get_playback_speed", [](AnimationWrapper& animation) -> double {
      return animation.Unwrap().GetPlaybackSpeed();
    },
    "refresh", [](AnimationWrapper& animation, WeakWrapper<SpriteProxyNode>& target) {
      animation.Unwrap().Refresh(target.Unwrap()->getSprite());
    },
    "copy_from", [](AnimationWrapper& animation, AnimationWrapper& rhs) {
      animation.Unwrap().CopyFrom(rhs.Unwrap());
    },
    "set_state", [](AnimationWrapper& animation, const std::string& state) {
      animation.Unwrap().SetAnimation(state);
    },
    "get_state", [](AnimationWrapper& animation) -> std::string {
      return animation.Unwrap().GetAnimationString();
    },
    "has_state", [](AnimationWrapper& animation, const std::string& state) -> bool {
      return animation.Unwrap().HasAnimation(state);
    },
    "point", [](AnimationWrapper& animation, const std::string& name) -> sf::Vector2f {
      return animation.Unwrap().GetPoint(name);
    },
    "set_playback", [](AnimationWrapper& animation, char mode) {
      animation.Unwrap() << mode;
    },

    // memory leak if animation created from lua is captured in these callback functions
    "on_complete", [] (AnimationWrapper& animation, sol::object callbackObject) {
      ExpectLuaFunction(callbackObject);

      animation.Unwrap() << [callbackObject] {
        sol::protected_function callback = callbackObject;
        auto result = callback();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(LogLevel::critical, error.what());
        }
      };
    },
    "on_frame", [](AnimationWrapper& animation, int frame, sol::object callbackObject, std::optional<bool> doOnce) {
      ExpectLuaFunction(callbackObject);

      animation.Unwrap().AddCallback(frame, [callbackObject] {
        sol::protected_function callback = callbackObject;
        auto result = callback();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(LogLevel::critical, error.what());
        }
      }, doOnce.value_or(false));
    },
    "on_interrupt", [](AnimationWrapper& animation, sol::object callbackObject) {
      ExpectLuaFunction(callbackObject);

      animation.Unwrap().SetInterruptCallback([callbackObject] {
        sol::protected_function callback = callbackObject;
        auto result = callback();

        if (!result.valid()) {
          sol::error error = result;
          Logger::Log(LogLevel::critical, error.what());
        }
      });
    }
  );

  state.new_enum("Playback",
    "Once", Animator::Mode::NoEffect,
    "Loop", Animator::Mode::Loop,
    "Bounce", Animator::Mode::Bounce,
    "Reverse", Animator::Mode::Reverse
  );
}
#endif
