use crate::prelude::*;
use std::path::PathBuf;

#[derive(Debug)]
pub(crate) enum InputEvent {
    Text(String),
    DropStart,
    DropCancelled,
    DroppedFile(PathBuf),
    #[allow(dead_code)] // winit needs support: https://github.com/rust-windowing/winit/issues/720
    DroppedText(String),
    MouseMoved {
        x: f32,
        y: f32,
    },
    MouseButtonDown(MouseButton),
    MouseButtonUp(MouseButton),
    KeyDown(Key),
    KeyUp(Key),
    ControllerConnected {
        controller_id: usize,
        rumble_pack: RumblePack,
    },
    ControllerDisconnected(usize),
    ControllerButtonDown {
        controller_id: usize,
        button: Button,
    },
    ControllerButtonUp {
        controller_id: usize,
        button: Button,
    },
    ControllerAxis {
        controller_id: usize,
        axis: AnalogAxis,
        value: f32,
    },
}
