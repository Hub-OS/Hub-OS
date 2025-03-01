use crate::ease::inverse_lerp;
use crate::render::ui::{FontName, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::RESOLUTION_F;
use framework::common::GameIO;
use std::rc::Rc;

const REVEAL_DURATION: FrameTime = 7;
const BOUNCE_BACK_DURATION: FrameTime = 3;
const TOTAL_TRANSITION_TIME: FrameTime = (REVEAL_DURATION + BOUNCE_BACK_DURATION) * 2;

#[derive(Clone, Copy)]
enum Mode {
    Hidden,
    OpenIndefinite,
    Close,
    Timed(FrameTime),
}

#[derive(Clone, Copy)]
pub enum BattleBannerMessage {
    TimeUp,
    TurnStart,
    TurnNumberStart(u32),
    FinalTurn,
    Failed,
    Success,
    BattleOver,
}

impl BattleBannerMessage {
    fn resolve_text(self) -> Rc<str> {
        match self {
            BattleBannerMessage::TimeUp => Rc::from("<_TIME_UP!_>"),
            BattleBannerMessage::TurnStart => Rc::from("<_TURN_START!_>"),
            BattleBannerMessage::TurnNumberStart(number) => {
                Rc::from(format!("<_TURN_{number}_START!_>"))
            }
            BattleBannerMessage::FinalTurn => Rc::from("<_FINAL_TURN!_>"),
            BattleBannerMessage::Failed => Rc::from("<_FAILED_>"),
            BattleBannerMessage::Success => Rc::from("<_SUCCESS_>"),
            BattleBannerMessage::BattleOver => Rc::from("<_BATTLE_OVER_>"),
        }
    }
}

#[derive(Clone)]
pub struct BattleBannerPopup {
    message: Rc<str>,
    time: FrameTime,
    mode: Mode,
    bottom: f32,
}

impl BattleBannerPopup {
    pub fn new(message: BattleBannerMessage) -> Self {
        Self {
            message: message.resolve_text(),
            time: 0,
            mode: Mode::Hidden,
            bottom: 0.0,
        }
    }

    pub fn with_bottom_position(mut self, bottom: f32) -> Self {
        self.bottom = bottom;
        self
    }

    pub fn show(&mut self) {
        self.mode = Mode::OpenIndefinite;
        self.time = 0;
    }

    pub fn show_for(&mut self, duration: FrameTime) {
        self.mode = Mode::Timed(duration.max(TOTAL_TRANSITION_TIME));
        self.time = 0;
    }

    pub fn close(&mut self) {
        self.mode = Mode::Close;
        self.time = 0;
    }

    pub fn update(&mut self) {
        self.time += 1;
    }

    pub fn remaining_time(&self) -> Option<FrameTime> {
        match self.mode {
            Mode::Hidden => None,
            Mode::OpenIndefinite => None,
            Mode::Close => {
                let end_time = BOUNCE_BACK_DURATION + REVEAL_DURATION;
                Some((end_time - self.time).max(0))
            }
            Mode::Timed(duration) => Some((duration - self.time).max(0)),
        }
    }

    fn resolve_message_scale(&self) -> f32 {
        let mut elapsed_time = self.time;
        let mut idle_duration = 0;

        match self.mode {
            Mode::Hidden => {
                return 0.0;
            }
            Mode::OpenIndefinite => {
                // set idle time to a ridiculously high number
                idle_duration = FrameTime::MAX;
            }
            Mode::Close => {
                // skip to the closing part
                elapsed_time += REVEAL_DURATION + BOUNCE_BACK_DURATION;
            }
            Mode::Timed(duration) => {
                idle_duration = duration - TOTAL_TRANSITION_TIME;
            }
        };

        fn reveal_scale(elapsed_time: FrameTime) -> f32 {
            inverse_lerp!(0, REVEAL_DURATION, elapsed_time) * 1.2
        }

        fn reveal_inverse_scale(elapsed_time: FrameTime) -> f32 {
            inverse_lerp!(REVEAL_DURATION, 0, elapsed_time) * 1.2
        }

        fn bounce_back_scale(elapsed_time: FrameTime) -> f32 {
            1.0 + inverse_lerp!(BOUNCE_BACK_DURATION, 0, elapsed_time) * 0.2
        }

        fn bounce_back_inverse_scale(elapsed_time: FrameTime) -> f32 {
            1.0 + inverse_lerp!(0, BOUNCE_BACK_DURATION, elapsed_time) * 0.2
        }

        fn idle_scale(_: FrameTime) -> f32 {
            1.0
        }

        let steps: [(FrameTime, fn(FrameTime) -> f32); 5] = [
            (REVEAL_DURATION, reveal_scale),
            (BOUNCE_BACK_DURATION, bounce_back_scale),
            (idle_duration, idle_scale),
            (BOUNCE_BACK_DURATION, bounce_back_inverse_scale),
            (REVEAL_DURATION, reveal_inverse_scale),
        ];

        for (duration, function) in steps {
            if elapsed_time <= duration {
                return function(elapsed_time);
            }

            elapsed_time -= duration;
        }

        0.0
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if matches!(self.mode, Mode::Hidden) {
            return;
        }

        let mut style = TextStyle::new(game_io, FontName::Battle);
        style.letter_spacing = 0.0;

        // measure before scaling
        let size = style.measure(&self.message).size;
        // scale
        style.scale.y = self.resolve_message_scale();

        // resolve position
        let mut position = RESOLUTION_F * 0.5 - size * style.scale * 0.5;
        // subtract unscaled size
        position.y -= size.y;
        style.bounds.set_position(position);

        style.draw(game_io, sprite_queue, &self.message);
    }
}
