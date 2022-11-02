use crate::prelude::*;
use winit::event::MouseButton as WinitMouseButton;
use winit::event::WindowEvent as WinitWindowEvent;

pub(crate) fn translate_winit_event(
    primary_window_id: winit::window::WindowId,
    event: winit::event::Event<()>,
) -> Option<WindowEvent> {
    match event {
        // todo check window id
        winit::event::Event::WindowEvent { window_id, event } => {
            if primary_window_id != window_id {
                return None;
            }

            match event {
                WinitWindowEvent::CloseRequested => Some(WindowEvent::CloseRequested),
                WinitWindowEvent::Resized(winit::dpi::PhysicalSize { width, height }) => {
                    Some(WindowEvent::Resized(UVec2::new(width, height)))
                }
                WinitWindowEvent::Moved(position) => {
                    Some(WindowEvent::Moved(IVec2::new(position.x, position.y)))
                }
                WinitWindowEvent::ReceivedCharacter(c) => {
                    let translated_char = match c {
                        '\r' => Some('\n'),
                        _ => Some(c),
                    };

                    translated_char.map(|c| InputEvent::Text(c.to_string()).into())
                }
                WinitWindowEvent::HoveredFile(_) => Some(InputEvent::DropStart.into()),
                WinitWindowEvent::HoveredFileCancelled => Some(InputEvent::DropCancelled.into()),
                WinitWindowEvent::DroppedFile(path_buf) => {
                    Some(InputEvent::DroppedFile(path_buf).into())
                }
                WinitWindowEvent::CursorMoved { position, .. } => Some(
                    InputEvent::MouseMoved {
                        x: position.x as f32,
                        y: position.y as f32,
                    }
                    .into(),
                ),
                WinitWindowEvent::MouseInput { state, button, .. } => {
                    if state == winit::event::ElementState::Pressed {
                        Some(
                            InputEvent::MouseButtonDown(translate_winit_mouse_button(button)?)
                                .into(),
                        )
                    } else {
                        Some(
                            InputEvent::MouseButtonUp(translate_winit_mouse_button(button)?).into(),
                        )
                    }
                }
                WinitWindowEvent::KeyboardInput {
                    input:
                        winit::event::KeyboardInput {
                            state,
                            virtual_keycode: Some(key),
                            ..
                        },
                    ..
                } => super::translate_winit_key(key).map(|key| {
                    if state == winit::event::ElementState::Pressed {
                        InputEvent::KeyDown(key).into()
                    } else {
                        InputEvent::KeyUp(key).into()
                    }
                }),
                _ => None,
            }
        }
        _ => None,
    }
}

fn translate_winit_mouse_button(button: WinitMouseButton) -> Option<MouseButton> {
    match button {
        WinitMouseButton::Left => Some(MouseButton::Left),
        WinitMouseButton::Middle => Some(MouseButton::Middle),
        WinitMouseButton::Right => Some(MouseButton::Right),
        _ => None,
    }
}
