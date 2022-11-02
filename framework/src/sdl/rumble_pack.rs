use std::cell::RefCell;
use std::time::Duration;

pub(crate) struct RumblePack {
  controller: RefCell<sdl2::controller::GameController>,
}

impl RumblePack {
  pub(super) fn new(controller: sdl2::controller::GameController) -> Self {
    Self {
      controller: RefCell::new(controller),
    }
  }

  pub fn rumble(&self, weak: f32, strong: f32, duration: Duration) {
    let _ = self.controller.borrow_mut().set_rumble(
      (weak.clamp(0.0, 1.0) * u16::MAX as f32) as u16,
      (strong.clamp(0.0, 1.0) * u16::MAX as f32) as u16,
      duration.as_millis() as u32,
    );
  }
}

impl std::fmt::Debug for RumblePack {
  fn fmt<'a>(&self, formatter: &mut std::fmt::Formatter<'a>) -> Result<(), std::fmt::Error> {
    formatter.write_str("RumblePack")
  }
}
