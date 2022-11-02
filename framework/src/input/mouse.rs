#[derive(PartialEq, Eq, Copy, Clone, Debug)]
pub enum MouseButton {
    Left,
    Middle,
    Right,
    Other(u16),
}
