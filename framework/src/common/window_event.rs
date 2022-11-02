use crate::prelude::*;

pub(crate) enum WindowEvent {
    CloseRequested,
    Resized(UVec2),
    Moved(IVec2),
    InputEvent(InputEvent),
}

impl std::convert::From<InputEvent> for WindowEvent {
    fn from(event: InputEvent) -> Self {
        WindowEvent::InputEvent(event)
    }
}
