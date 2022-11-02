#[derive(Default)]
pub(crate) struct WindowConfig {
    pub title: String,
    pub size: (u32, u32),
    pub resolution: Option<(u32, u32)>,
    pub borderless: bool,
    pub fullscreen: bool,
    pub resizable: bool,
    #[allow(dead_code)] // not available with sdl
    pub always_on_top: bool,
    #[allow(dead_code)] // not available with sdl
    pub transparent: bool,
}
