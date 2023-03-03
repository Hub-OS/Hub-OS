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
    Map,
    FaceLeft,
    FaceRight,
    AdvanceFrame,
    RewindFrame,
}

impl Input {
    pub const REQUIRED: [Input; 10] = [
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

    pub const NON_OVERLAP: [Input; 9] = [
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

    pub const BATTLE: [Input; 15] = [
        Input::Up,
        Input::Down,
        Input::Left,
        Input::Right,
        Input::Shoot,
        Input::UseCard,
        Input::Special,
        Input::FaceLeft,
        Input::FaceRight,
        Input::ShoulderL,
        Input::ShoulderR,
        Input::End,
        Input::EndTurn,
        Input::Confirm,
        Input::Cancel,
    ];

    pub const REPEATABLE: [Input; 6] = [
        Input::Up,
        Input::Down,
        Input::Left,
        Input::Right,
        Input::ShoulderL,
        Input::ShoulderR,
    ];
}
