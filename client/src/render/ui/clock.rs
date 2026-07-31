use super::{FontName, TextStyle};
use crate::render::*;
use crate::resources::*;
use crate::saves::DateFormat;
use crate::saves::TimeFormat;
use chrono::Timelike;
use framework::prelude::*;
use std::fmt::Write;

const FONT: FontName = FontName::Thick;
const TEXT_SHADOW_COLOR: Color = Color::new(0.41, 0.41, 0.41, 1.0);
const TEXT_SHADOW_OFFSET: f32 = 1.0;
const MARGIN: f32 = 2.0;

pub fn draw_clock(game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
    const MARGIN: f32 = 2.0;

    let globals = Globals::from_resources(game_io);
    let config = &globals.config;

    // generate initial text
    let time = chrono::Local::now();
    let mut full_text = String::new();

    // hour
    let formatted_hour = match config.time_format {
        TimeFormat::Twelve => time.hour12().1,
        TimeFormat::TwentyFour => time.hour(),
    };
    let _ = write!(&mut full_text, "{formatted_hour:0>2}");

    // colon
    full_text.push(if time.timestamp_subsec_millis() > 500 {
        ':'
    } else {
        ' '
    });

    // minute
    let _ = write!(&mut full_text, "{:0>2}", time.minute());

    // AM/PM
    let is_afternoon = time.hour() >= 12;

    if is_afternoon {
        full_text.push_str(" PM");
    } else {
        full_text.push_str(" AM");
    }

    // calculate the position
    let white_str = &full_text[..5];
    let am_pm_str = &full_text[5..];

    let mut time_style = TextStyle::new_monospace(game_io, FONT);
    let text_size = time_style.measure(&full_text).size;

    time_style.bounds.set_position(Vec2::new(
        RESOLUTION_F.x - text_size.x + TEXT_SHADOW_OFFSET - MARGIN,
        TEXT_SHADOW_OFFSET + MARGIN,
    ));

    // draw the shadow
    time_style.color = TEXT_SHADOW_COLOR;
    time_style.draw(game_io, sprite_queue, &full_text);

    // draw the time in white
    time_style.bounds -= Vec2::new(TEXT_SHADOW_OFFSET, TEXT_SHADOW_OFFSET);

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

pub fn draw_date(game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
    let globals = Globals::from_resources(game_io);
    let config = &globals.config;

    let text = match config.date_format {
        DateFormat::Auto => {
            let time = libc_strftime::epoch();
            libc_strftime::strftime_local("%x", time)
        }
        DateFormat::Dmy => {
            let time = chrono::Local::now();
            time.format("%d/%m/%y").to_string()
        }
        DateFormat::Mdy => {
            let time = chrono::Local::now();
            time.format("%m/%d/%y").to_string()
        }
    };

    let mut time_style = TextStyle::new_monospace(game_io, FONT);
    time_style.shadow_color = TEXT_SHADOW_COLOR;
    time_style.bounds.set_position(Vec2::new(MARGIN, MARGIN));
    time_style.draw(game_io, sprite_queue, &text);
}
