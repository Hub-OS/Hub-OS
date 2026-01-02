use super::{FontName, TextStyle};
use crate::render::SpriteColorQueue;
use crate::resources::Globals;
use framework::logging::LogRecord;
use framework::prelude::*;

struct LogLine {
    is_tip: bool,
    level: log::Level,
    message: String,
}

pub struct LogBox {
    bounds: Rect,
    text_style: TextStyle,
    tip_prefix: String,
    lines: Vec<LogLine>,
    error_count: usize,
    warning_count: usize,
}

impl LogBox {
    pub fn new(game_io: &GameIO, bounds: Rect) -> Self {
        let mut text_style = TextStyle::new(game_io, FontName::Thin);
        text_style.scale = Vec2::new(0.5, 0.5);

        let globals = Globals::from_resources(game_io);
        let tip_prefix = globals.translate_with_args("tip-prefixed", vec![("tip", "".into())]);

        Self {
            text_style,
            bounds,
            tip_prefix,
            lines: Vec::new(),
            error_count: 0,
            warning_count: 0,
        }
    }

    pub fn error_count(&self) -> usize {
        self.error_count
    }

    pub fn warning_count(&self) -> usize {
        self.warning_count
    }

    pub fn push_space(&mut self) {
        self.lines.push(LogLine {
            is_tip: false,
            level: log::Level::Info,
            message: String::new(),
        })
    }

    pub fn push_record(&mut self, record: LogRecord) {
        self.text_style.bounds = self.bounds;
        let text_measurement = self.text_style.measure(&record.message);

        match record.level {
            log::Level::Error => {
                self.error_count += 1;
            }
            log::Level::Warn => {
                self.warning_count += 1;
            }
            _ => {}
        }

        let is_tip = record.message.starts_with(&self.tip_prefix);

        if is_tip {
            // add a space before the tip
            self.push_space();
        }

        let new_records = text_measurement.line_ranges.iter().map(|range| LogLine {
            is_tip,
            level: record.level,
            message: record.message[range.clone()].to_string(),
        });

        self.lines.extend(new_records);

        if is_tip {
            // add a space after the tip
            self.push_space();
        }
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.text_style.bounds.x = self.bounds.x;

        let line_height = self.text_style.line_height();
        let max_lines = (self.bounds.height / line_height) as usize;

        let bottom = self.bounds.top() + line_height * self.lines.len().min(max_lines) as f32;

        for (i, line) in self.lines.iter().rev().enumerate() {
            if i >= max_lines {
                break;
            }

            let color = match line.level {
                log::Level::Error => Color::RED,
                log::Level::Warn => Color::YELLOW,
                log::Level::Trace | log::Level::Debug => Color::new(0.5, 0.5, 0.5, 1.0),
                _ => {
                    if line.is_tip {
                        Color::GREEN
                    } else {
                        Color::WHITE
                    }
                }
            };

            self.text_style.color = color;

            self.text_style.bounds.y = bottom - i as f32 * line_height - line_height;

            self.text_style.draw(game_io, sprite_queue, &line.message);
        }
    }
}
