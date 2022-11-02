use super::WindowLoop;
use crate::prelude::*;
use winit::{dpi::PhysicalSize, event_loop::EventLoop, window::WindowBuilder};

pub struct Window {
    window: winit::window::Window,
    position: IVec2,
    size: UVec2,
    resolution: UVec2,
    static_resolution: bool,
}

impl Window {
    pub fn position(&self) -> IVec2 {
        self.position
    }

    pub fn set_position(&mut self, position: IVec2) {
        use winit::dpi::LogicalPosition;
        self.window
            .set_outer_position(LogicalPosition::new(position.x, position.y));
        self.position = position;
    }

    pub fn fullscreen(&self) -> bool {
        self.window.fullscreen().is_some()
    }

    pub fn set_fullscreen(&mut self, fullscreen: bool) {
        use winit::window::Fullscreen;

        let mode = if fullscreen {
            Some(Fullscreen::Borderless(None))
        } else {
            None
        };

        self.window.set_fullscreen(mode);
    }

    pub fn size(&self) -> UVec2 {
        self.size
    }

    pub fn resolution(&self) -> UVec2 {
        self.resolution
    }

    pub fn set_title(&mut self, title: &str) {
        let _ = self.window.set_title(title);
    }

    pub(crate) fn build(window_config: WindowConfig) -> anyhow::Result<WindowLoop> {
        let event_loop = EventLoop::new();
        let mut winit_window_builder = WindowBuilder::new()
            .with_title(&window_config.title)
            .with_inner_size(PhysicalSize::new(
                window_config.size.0,
                window_config.size.1,
            ))
            .with_resizable(window_config.resizable)
            .with_decorations(!window_config.borderless)
            .with_transparent(window_config.transparent)
            .with_always_on_top(window_config.always_on_top);

        if window_config.fullscreen {
            use winit::window::Fullscreen;
            winit_window_builder =
                winit_window_builder.with_fullscreen(Some(Fullscreen::Borderless(None)));
        }

        let winit_window = winit_window_builder.build(&event_loop)?;

        crate::cfg_web!({
            use wasm_bindgen::JsCast;
            use web_sys::{Element, HtmlCanvasElement};
            use winit::platform::web::WindowExtWebSys;

            let document = web_sys::window().unwrap().document().unwrap();

            let canvas = Element::from(winit_window.canvas())
                .dyn_into::<HtmlCanvasElement>()
                .unwrap();

            canvas.style().set_property("outline", "0").unwrap();

            document
                .body()
                .unwrap()
                .append_child(&canvas)
                .expect("Couldn't append canvas to document body.");
        });

        let position = winit_window.outer_position().unwrap_or_default();

        let window = Window {
            window: winit_window,
            position: IVec2::new(position.x, position.y),
            size: window_config.size.into(),
            resolution: window_config
                .resolution
                .unwrap_or(window_config.size)
                .into(),
            static_resolution: window_config.resolution.is_some(),
        };

        let window_loop = WindowLoop::new(window, event_loop);

        Ok(window_loop)
    }

    pub(crate) fn id(&self) -> winit::window::WindowId {
        self.window.id()
    }

    pub(crate) fn moved(&mut self, position: IVec2) {
        self.position = position;
    }

    pub(crate) fn resized(&mut self, size: UVec2) {
        self.size = size;

        if !self.static_resolution {
            self.resolution = size;
        }
    }

    pub(crate) fn set_text_input(&mut self, accept: bool) {
        self.window.set_ime_allowed(accept);
    }
}

use raw_window_handle::{
    HasRawDisplayHandle, HasRawWindowHandle, RawDisplayHandle, RawWindowHandle,
};

unsafe impl HasRawWindowHandle for Window {
    fn raw_window_handle(&self) -> RawWindowHandle {
        self.window.raw_window_handle()
    }
}

unsafe impl HasRawDisplayHandle for Window {
    fn raw_display_handle(&self) -> RawDisplayHandle {
        self.window.raw_display_handle()
    }
}
