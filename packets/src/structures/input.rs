use num_derive::FromPrimitive;
use serde::{Deserialize, Serialize};
use std::hash::Hash;
use strum::{EnumCount, EnumIter, IntoStaticStr};

#[repr(u8)]
#[derive(
    Serialize,
    Deserialize,
    Clone,
    Copy,
    Hash,
    PartialEq,
    Eq,
    Debug,
    EnumCount,
    EnumIter,
    FromPrimitive,
    IntoStaticStr,
)]
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
    Option2, // could use a better name
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

    pub const NON_OVERLAP: [Input; 10] = [
        Input::Up,
        Input::Down,
        Input::Left,
        Input::Right,
        Input::Cancel,
        Input::Confirm,
        Input::Option,
        Input::Option2,
        Input::ShoulderL,
        Input::ShoulderR,
    ];

    pub const BATTLE: [Input; 17] = [
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
        Input::Info,
        Input::Flee,
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
