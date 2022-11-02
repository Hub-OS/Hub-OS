use super::translate_sdl_key;
use crate::prelude::*;
use sdl2::controller::Axis as SDLAxis;
use sdl2::controller::Button as SDLButton;
use sdl2::event::{Event as SDLEvent, WindowEvent as SDLWindowEvent};
use sdl2::mouse::MouseButton as SDLMouseButton;

pub(crate) fn translate_sdl_event(game_window_id: u32, event: SDLEvent) -> Option<WindowEvent> {
    match event {
        SDLEvent::Quit { .. } => Some(WindowEvent::CloseRequested),
        SDLEvent::Window {
            window_id,
            win_event,
            ..
        } if game_window_id == window_id => match win_event {
            SDLWindowEvent::SizeChanged(width, height) => Some(WindowEvent::Resized(UVec2::new(
                width as u32,
                height as u32,
            ))),
            SDLWindowEvent::Moved(x, y) => Some(WindowEvent::Moved(IVec2::new(x, y))),
            _ => None,
        },
        SDLEvent::TextInput {
            window_id, text, ..
        } => {
            if game_window_id == window_id {
                Some(InputEvent::Text(text).into())
            } else {
                None
            }
        }
        SDLEvent::DropBegin { window_id, .. } => {
            if game_window_id == window_id {
                Some(InputEvent::DropStart.into())
            } else {
                None
            }
        }
        SDLEvent::DropComplete { window_id, .. } => {
            if game_window_id == window_id {
                Some(InputEvent::DropCancelled.into())
            } else {
                None
            }
        }
        SDLEvent::DropFile {
            filename,
            window_id,
            ..
        } => {
            if game_window_id == window_id {
                Some(InputEvent::DroppedFile(filename.into()).into())
            } else {
                None
            }
        }
        SDLEvent::DropText {
            filename,
            window_id,
            ..
        } => {
            if game_window_id == window_id {
                Some(InputEvent::DroppedText(filename).into())
            } else {
                None
            }
        }
        SDLEvent::MouseMotion {
            window_id, x, y, ..
        } => {
            if game_window_id == window_id {
                Some(
                    InputEvent::MouseMoved {
                        x: x as f32,
                        y: y as f32,
                    }
                    .into(),
                )
            } else {
                None
            }
        }
        SDLEvent::MouseButtonDown {
            window_id,
            mouse_btn,
            ..
        } => {
            if game_window_id == window_id {
                Some(InputEvent::MouseButtonDown(translate_sdl_mouse_button(mouse_btn)?).into())
            } else {
                None
            }
        }
        SDLEvent::MouseButtonUp {
            window_id,
            mouse_btn,
            ..
        } => {
            if game_window_id == window_id {
                Some(InputEvent::MouseButtonUp(translate_sdl_mouse_button(mouse_btn)?).into())
            } else {
                None
            }
        }
        SDLEvent::KeyDown {
            window_id, keycode, ..
        } => {
            if game_window_id == window_id {
                keycode
                    .and_then(translate_sdl_key)
                    .map(|key| WindowEvent::InputEvent(InputEvent::KeyDown(key)))
            } else {
                None
            }
        }
        SDLEvent::KeyUp {
            window_id, keycode, ..
        } => {
            if game_window_id == window_id {
                keycode
                    .and_then(translate_sdl_key)
                    .map(|key| WindowEvent::InputEvent(InputEvent::KeyUp(key)))
            } else {
                None
            }
        }
        SDLEvent::ControllerDeviceRemoved { which, .. } => {
            Some(InputEvent::ControllerDisconnected(which as usize).into())
        }
        SDLEvent::ControllerButtonDown { which, button, .. } => Some(
            InputEvent::ControllerButtonDown {
                controller_id: which as usize,
                button: convert_button(button)?,
            }
            .into(),
        ),
        SDLEvent::ControllerButtonUp { which, button, .. } => Some(
            InputEvent::ControllerButtonUp {
                controller_id: which as usize,
                button: convert_button(button)?,
            }
            .into(),
        ),
        SDLEvent::ControllerAxisMotion {
            which, axis, value, ..
        } => Some({
            let axis = convert_axis(axis);
            let mut value = ((value as f32) / 32767.0).max(-1.0);

            if axis.is_y_axis() {
                value = -value;
            }

            InputEvent::ControllerAxis {
                controller_id: which as usize,
                axis,
                value,
            }
            .into()
        }),
        SDLEvent::FingerDown { finger_id, .. } => {
            println!("{}", finger_id);
            None
        }
        SDLEvent::FingerUp { finger_id, .. } => {
            println!("{}", finger_id);
            None
        }
        _ => None,
    }
}

fn convert_button(button: SDLButton) -> Option<Button> {
    match button {
        SDLButton::A => Some(Button::A),
        SDLButton::B => Some(Button::B),
        SDLButton::X => Some(Button::X),
        SDLButton::Y => Some(Button::Y),
        SDLButton::Guide => Some(Button::Meta),
        SDLButton::Back => Some(Button::Select),
        SDLButton::Start => Some(Button::Start),
        SDLButton::LeftStick => Some(Button::LeftStick),
        SDLButton::RightStick => Some(Button::RightStick),
        SDLButton::LeftShoulder => Some(Button::LeftShoulder),
        SDLButton::RightShoulder => Some(Button::RightShoulder),
        SDLButton::DPadUp => Some(Button::DPadUp),
        SDLButton::DPadDown => Some(Button::DPadDown),
        SDLButton::DPadLeft => Some(Button::DPadLeft),
        SDLButton::DPadRight => Some(Button::DPadRight),
        SDLButton::Paddle1 => Some(Button::Paddle1),
        SDLButton::Paddle2 => Some(Button::Paddle2),
        SDLButton::Paddle3 => Some(Button::Paddle3),
        SDLButton::Paddle4 => Some(Button::Paddle4),
        _ => None,
    }
}

fn convert_axis(axis: SDLAxis) -> AnalogAxis {
    match axis {
        SDLAxis::LeftX => AnalogAxis::LeftStickX,
        SDLAxis::LeftY => AnalogAxis::LeftStickY,
        SDLAxis::RightX => AnalogAxis::RightStickX,
        SDLAxis::RightY => AnalogAxis::RightStickY,
        SDLAxis::TriggerLeft => AnalogAxis::LeftTrigger,
        SDLAxis::TriggerRight => AnalogAxis::RightTrigger,
    }
}

fn translate_sdl_mouse_button(button: SDLMouseButton) -> Option<MouseButton> {
    match button {
        SDLMouseButton::Left => Some(MouseButton::Left),
        SDLMouseButton::Middle => Some(MouseButton::Middle),
        SDLMouseButton::Right => Some(MouseButton::Right),
        _ => None,
    }
}
