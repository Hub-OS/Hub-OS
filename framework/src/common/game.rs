use crate::prelude::*;

type GlobalsConstructor<Globals> = Box<dyn FnOnce(&mut GameIO<Globals>) -> Globals>;
type SceneConstructor<Globals> = Box<dyn FnOnce(&mut GameIO<Globals>) -> Box<dyn Scene<Globals>>>;
type OverlayConstructor<Globals> =
    Box<dyn FnOnce(&mut GameIO<Globals>) -> Box<dyn SceneOverlay<Globals>>>;

pub struct Game<Globals: 'static> {
    globals_constructor: GlobalsConstructor<Globals>,
    window_config: WindowConfig,
    target_fps: u16,
    overlay_constructor: Option<OverlayConstructor<Globals>>,
}

impl<Globals> Game<Globals> {
    pub fn new(
        title: &str,
        size: (u32, u32),
        globals_constructor: impl FnOnce(&mut GameIO<Globals>) -> Globals + 'static,
    ) -> Game<Globals> {
        Game {
            globals_constructor: Box::new(globals_constructor),
            target_fps: 60,
            window_config: WindowConfig {
                title: String::from(title),
                size,
                ..Default::default()
            },
            overlay_constructor: None,
        }
    }

    pub fn with_borderless(mut self, value: bool) -> Self {
        self.window_config.borderless = value;
        self
    }

    pub fn with_fullscreen(mut self, value: bool) -> Self {
        self.window_config.fullscreen = value;
        self
    }

    pub fn with_resizable(mut self, value: bool) -> Self {
        self.window_config.resizable = value;
        self
    }

    pub fn with_transparent(mut self, value: bool) -> Self {
        self.window_config.transparent = value;
        self
    }

    pub fn with_always_on_top(mut self, value: bool) -> Self {
        self.window_config.always_on_top = value;
        self
    }

    pub fn with_resolution(mut self, resolution: (u32, u32)) -> Self {
        self.window_config.resolution = Some(resolution);
        self
    }

    pub fn with_target_fps(mut self, target_fps: u16) -> Self {
        self.target_fps = target_fps;
        self
    }

    pub fn with_overlay<OverlayConstructor>(
        mut self,
        overlay_constuctor: OverlayConstructor,
    ) -> Self
    where
        OverlayConstructor:
            FnOnce(&mut GameIO<Globals>) -> Box<dyn SceneOverlay<Globals>> + 'static,
    {
        self.overlay_constructor = Some(Box::new(overlay_constuctor));
        self
    }

    pub fn run<SceneConstructor>(self, scene_constructor: SceneConstructor) -> anyhow::Result<()>
    where
        SceneConstructor: FnOnce(&mut GameIO<Globals>) -> Box<dyn Scene<Globals>> + 'static,
    {
        let window_loop = Window::build(self.window_config)?;

        let params = WindowLoopParams {
            globals_constructor: self.globals_constructor,
            scene_constructor: Box::new(scene_constructor),
            target_fps: self.target_fps,
            overlay_constructor: self.overlay_constructor,
        };

        pollster::block_on(window_loop.run(params))
    }
}

pub(crate) struct WindowLoopParams<Globals: 'static> {
    pub globals_constructor: GlobalsConstructor<Globals>,
    pub scene_constructor: SceneConstructor<Globals>,
    pub target_fps: u16,
    pub overlay_constructor: Option<OverlayConstructor<Globals>>,
}
