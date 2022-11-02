mod game;
mod game_io;
mod game_runtime;
mod next_scene;
mod ortho_camera;
mod scene;
mod scene_manager;
mod scene_overlay;
mod transition;
mod window_config;
mod window_event;

pub(crate) use game_runtime::*;
pub(super) use scene_manager::*;
pub(crate) use window_config::*;
pub(crate) use window_event::*;

pub use game::*;
pub use game_io::*;
pub use next_scene::*;
pub use ortho_camera::*;
pub use scene::*;
pub use scene_overlay::*;
pub use transition::*;

#[cfg(feature = "sdl")]
crate::cfg_native! {
  pub use crate::sdl::*;
}

#[cfg(feature = "winit")]
crate::cfg_native! {
  pub use crate::winit::*;
}

crate::cfg_web! {
  pub use crate::winit::*;
}
