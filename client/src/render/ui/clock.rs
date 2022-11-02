use super::{FontStyle, TextStyle};
use crate::render::*;
use crate::resources::*;
use chrono::Timelike;
use framework::prelude::*;

const TEXT_SHADOW_COLOR: Color = Color::new(0.41, 0.41, 0.41, 1.0);
const TEXT_SHADOW_OFFSET: f32 = 1.0;

pub fn draw_clock(game_io: &GameIO<Globals>, sprite_queue: &mut SpriteColorQueue) {
    const SHADOW_OFFSET: f32 = 1.0;
    const MARGIN: f32 = 2.0;

    // generate initial text
    let time = chrono::Local::now();
    let is_afternoon = time.hour() >= 12;
    let show_colon = time.timestamp_subsec_millis() > 500;

    let mut time_style = TextStyle::new(game_io, FontStyle::Thick);
    let format = match show_colon {
        false => "%I %M %p",
        true => "%I:%M %p",
    };

    // calculate the position
    let full_text = time.format(format).to_string();
    let white_str = &full_text[..5];
    let am_pm_str = &full_text[5..];

    let text_size = time_style.measure(&full_text).size;

    time_style.bounds.set_position(Vec2::new(
        RESOLUTION_F.x - text_size.x + TEXT_SHADOW_OFFSET - MARGIN,
        TEXT_SHADOW_OFFSET + MARGIN,
    ));

    // draw the shadow
    time_style.color = TEXT_SHADOW_COLOR;
    time_style.draw(game_io, sprite_queue, &full_text);

    // draw the time in white
    time_style.bounds -= Vec2::new(SHADOW_OFFSET, SHADOW_OFFSET);

    time_style.color = Color::WHITE;
    time_style.draw(game_io, sprite_queue, white_str);

    // draw the AM/PM
    let text_size = time_style.measure(white_str).size;
    time_style.bounds.x += text_size.x + time_style.letter_spacing;

    let am_pm_color = match is_afternoon {
        false => Color::RED,
        true => Color::GREEN,
    };

    time_style.color = am_pm_color;
    time_style.draw(game_io, sprite_queue, am_pm_str);
}
