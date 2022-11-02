use gilrs::{ff, GamepadId, Gilrs};
use std::cell::RefCell;
use std::rc::Rc;
use std::time::Duration;

#[derive(Debug)]
pub(crate) struct RumblePack {
  gilrs: Rc<RefCell<Gilrs>>,
  gamepad_id: GamepadId,
}

impl RumblePack {
  pub(super) fn new(gilrs: Rc<RefCell<Gilrs>>, gamepad_id: GamepadId) -> Self {
    Self { gilrs, gamepad_id }
  }

  pub fn rumble(&self, weak: f32, strong: f32, duration: Duration) {
    // https://gitlab.com/gilrs-project/gilrs#supported-features
    // no wasm support yet, appears to require threads
    crate::cfg_native!({
      let mut weak_effect = ff::BaseEffect {
        kind: ff::BaseEffectType::Weak {
          magnitude: (weak.clamp(0.0, 1.0) * u16::MAX as f32) as u16,
        },
        ..Default::default()
      };

      let mut strong_effect = ff::BaseEffect {
        kind: ff::BaseEffectType::Strong {
          magnitude: (strong.clamp(0.0, 1.0) * u16::MAX as f32) as u16,
        },
        ..Default::default()
      };

      let scheduling = ff::Replay {
        play_for: ff::Ticks::from_ms(duration.as_millis() as u32),
        ..Default::default()
      };

      let duration_ms = duration.as_millis() as f32;

      let envelope = ff::Envelope {
        attack_length: ff::Ticks::from_ms((duration_ms * 0.25) as u32),
        fade_length: ff::Ticks::from_ms((duration_ms * 0.075) as u32),
        attack_level: 0.5,
        fade_level: -0.5,
      };

      weak_effect.scheduling = scheduling;
      strong_effect.scheduling = scheduling;
      weak_effect.envelope = envelope;
      strong_effect.envelope = envelope;

      let effect_result = ff::EffectBuilder::new()
        .add_effect(weak_effect)
        .add_effect(strong_effect)
        .gamepads(&[self.gamepad_id])
        .finish(&mut *self.gilrs.borrow_mut());

      if let Ok(effect) = effect_result {
        std::thread::spawn(move || {
          let _ = effect.play();
          std::thread::sleep(duration);
        });
      }
    });
  }
}
