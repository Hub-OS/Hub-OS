use num_derive::FromPrimitive;
use std::hash::Hash;
use strum::{EnumIter, IntoStaticStr};

#[repr(u8)]
#[derive(Clone, Copy, Hash, PartialEq, Eq, Debug, EnumIter, FromPrimitive, IntoStaticStr)]
pub enum Input {
    Up,
    Down,
    Left,
    Right,
    UseCard,
    Shoot,
    Special,
    Flee,
    EndTurn,
    Pause,
    Confirm,
    Cancel,
    Option,
    Info,
    End,
    Sprint,
    ShoulderL,
    ShoulderR,
    Minimap,
    FaceLeft,
    FaceRight,
    AdvanceFrame,
    RewindFrame,
}

impl Input {
    pub const REQUIRED_INPUTS: [Input; 10] = [
        Input::Up,
        Input::Down,
        Input::Left,
        Input::Right,
        Input::Cancel,
        Input::Confirm,
        Input::Pause,
        Input::Option,
        Input::ShoulderL,
        Input::ShoulderR,
    ];

    pub const NON_OVERLAP_INPUTS: [Input; 9] = [
        Input::Up,
        Input::Down,
        Input::Left,
        Input::Right,
        Input::Cancel,
        Input::Confirm,
        Input::Option,
        Input::ShoulderL,
        Input::ShoulderR,
    ];
}
