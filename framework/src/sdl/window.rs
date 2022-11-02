use crate::prelude::*;

pub struct Window {
    window: sdl2::video::Window,
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
        use sdl2::video::WindowPos;
        self.window.set_position(
            WindowPos::Positioned(position.x),
            WindowPos::Positioned(position.y),
        );
        self.position = position;
    }

    pub fn fullscreen(&self) -> bool {
        use sdl2::video::FullscreenType;

        !matches!(self.window.fullscreen_state(), FullscreenType::Off)
    }

    pub fn set_fullscreen(&mut self, fullscreen: bool) {
        use sdl2::video::FullscreenType;

        let mode = if fullscreen {
            FullscreenType::Desktop
        } else {
            FullscreenType::Off
        };

        let _ = self.window.set_fullscreen(mode);
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
        let sdl_context = sdl2::init().map_err(|e| anyhow::anyhow!(e))?;
        let event_pump = sdl_context.event_pump().map_err(|e| anyhow::anyhow!(e))?;

        let video_subsystem = sdl_context.video().map_err(|e| anyhow::anyhow!(e))?;
        let mut sdl_window_builder = video_subsystem.window(
            &window_config.title,
            window_config.size.0,
            window_config.size.1,
        );

        sdl_window_builder.position_centered();

        if window_config.resizable {
            sdl_window_builder.resizable();
        }

        if window_config.fullscreen {
            sdl_window_builder.fullscreen();
        }

        if window_config.borderless {
            sdl_window_builder.borderless();
        }

        let sdl_window = sdl_window_builder.build()?;

        let size = sdl_window.size();

        let window = Window {
            size: sdl_window.size().into(),
            position: sdl_window.position().into(),
            window: sdl_window,
            static_resolution: window_config.resolution.is_some(),
            resolution: window_config.resolution.unwrap_or(size).into(),
        };

        let game_controller_subsystem = sdl_context
            .game_controller()
            .map_err(|e| anyhow::anyhow!(e))?;

        let window_loop = WindowLoop::new(window, event_pump, game_controller_subsystem);

        Ok(window_loop)
    }

    pub(crate) fn id(&self) -> u32 {
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
        let text_input = self.window.subsystem().text_input();

        if accept {
            text_input.start();
        } else {
            text_input.stop();
        }
    }
}

use raw_window_handle::{
    HasRawDisplayHandle, HasRawWindowHandle, RawDisplayHandle, RawWindowHandle,
};

unsafe impl HasRawWindowHandle for Window {
    fn raw_window_handle(&self) -> RawWindowHandle {
        use raw_window_handle_0_4::HasRawWindowHandle;

        match self.window.raw_window_handle() {
            raw_window_handle_0_4::RawWindowHandle::UiKit(handle) => {
                let mut translated = raw_window_handle::UiKitWindowHandle::empty();
                translated.ui_view = handle.ui_view;
                translated.ui_view_controller = handle.ui_view_controller;
                translated.ui_window = handle.ui_window;

                RawWindowHandle::UiKit(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::AppKit(handle) => {
                let mut translated = raw_window_handle::AppKitWindowHandle::empty();
                translated.ns_window = handle.ns_window;
                translated.ns_view = handle.ns_view;

                RawWindowHandle::AppKit(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Orbital(handle) => {
                let mut translated = raw_window_handle::OrbitalWindowHandle::empty();
                translated.window = handle.window;

                RawWindowHandle::Orbital(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Xlib(handle) => {
                let mut translated = raw_window_handle::XlibWindowHandle::empty();
                translated.window = handle.window;
                translated.visual_id = handle.visual_id;

                RawWindowHandle::Xlib(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Xcb(handle) => {
                let mut translated = raw_window_handle::XcbWindowHandle::empty();
                translated.window = handle.window;
                translated.visual_id = handle.visual_id;

                RawWindowHandle::Xcb(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Wayland(handle) => {
                let mut translated = raw_window_handle::WaylandWindowHandle::empty();
                translated.surface = handle.surface;

                RawWindowHandle::Wayland(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Win32(handle) => {
                let mut translated = raw_window_handle::Win32WindowHandle::empty();
                translated.hwnd = handle.hwnd;
                translated.hinstance = handle.hinstance;

                RawWindowHandle::Win32(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::WinRt(handle) => {
                let mut translated = raw_window_handle::WinRtWindowHandle::empty();
                translated.core_window = handle.core_window;

                RawWindowHandle::WinRt(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Web(handle) => {
                let mut translated = raw_window_handle::WebWindowHandle::empty();
                translated.id = handle.id;

                RawWindowHandle::Web(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::AndroidNdk(handle) => {
                let mut translated = raw_window_handle::AndroidNdkWindowHandle::empty();
                translated.a_native_window = handle.a_native_window;

                RawWindowHandle::AndroidNdk(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Haiku(handle) => {
                let mut translated = raw_window_handle::HaikuWindowHandle::empty();
                translated.b_window = handle.b_window;
                translated.b_direct_window = handle.b_direct_window;

                RawWindowHandle::Haiku(translated)
            }
            _ => todo!(),
        }
    }
}

unsafe impl HasRawDisplayHandle for Window {
    fn raw_display_handle(&self) -> RawDisplayHandle {
        use raw_window_handle_0_4::HasRawWindowHandle;

        match self.window.raw_window_handle() {
            raw_window_handle_0_4::RawWindowHandle::UiKit(_) => {
                let translated = raw_window_handle::UiKitDisplayHandle::empty();

                RawDisplayHandle::UiKit(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::AppKit(_) => {
                let translated = raw_window_handle::AppKitDisplayHandle::empty();

                RawDisplayHandle::AppKit(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Orbital(_) => {
                let translated = raw_window_handle::OrbitalDisplayHandle::empty();

                RawDisplayHandle::Orbital(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Xlib(handle) => {
                let mut translated = raw_window_handle::XlibDisplayHandle::empty();
                translated.display = handle.display;
                // translated.screen = ?

                RawDisplayHandle::Xlib(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Xcb(handle) => {
                let mut translated = raw_window_handle::XcbDisplayHandle::empty();
                translated.connection = handle.connection;
                // translated.screen = ?

                RawDisplayHandle::Xcb(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Wayland(handle) => {
                let mut translated = raw_window_handle::WaylandDisplayHandle::empty();
                translated.display = handle.display;

                RawDisplayHandle::Wayland(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Win32(_)
            | raw_window_handle_0_4::RawWindowHandle::WinRt(_) => {
                let translated = raw_window_handle::WindowsDisplayHandle::empty();

                RawDisplayHandle::Windows(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Web(_) => {
                let translated = raw_window_handle::WebDisplayHandle::empty();

                RawDisplayHandle::Web(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::AndroidNdk(_) => {
                let translated = raw_window_handle::AndroidDisplayHandle::empty();

                RawDisplayHandle::Android(translated)
            }
            raw_window_handle_0_4::RawWindowHandle::Haiku(_) => {
                let translated = raw_window_handle::HaikuDisplayHandle::empty();

                RawDisplayHandle::Haiku(translated)
            }
            _ => todo!(),
        }
    }
}
