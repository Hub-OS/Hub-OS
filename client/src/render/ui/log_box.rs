use super::{FontName, TextStyle};
use crate::render::SpriteColorQueue;
use framework::logging::{LogLevel, LogRecord};
use framework::prelude::*;

pub struct LogBox {
    bounds: Rect,
    text_style: TextStyle,
    records: Vec<LogRecord>,
    error_count: usize,
    warning_count: usize,
}

impl LogBox {
    pub fn new(game_io: &GameIO, bounds: Rect) -> Self {
        let mut text_style = TextStyle::new(game_io, FontName::Thin);
        text_style.scale = Vec2::new(0.5, 0.5);

        Self {
            text_style,
            bounds,
            records: Vec::new(),
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

    pub fn push_record(&mut self, record: LogRecord) {
        self.text_style.bounds = self.bounds;
        let text_measurement = self.text_style.measure(&record.message);

        match record.level {
            LogLevel::Error => {
                self.error_count += 1;
            }
            LogLevel::Warn => {
                self.warning_count += 1;
            }
            _ => {}
        }

        let new_records = text_measurement.line_ranges.iter().map(|range| LogRecord {
            level: record.level,
            message: record.message[range.clone()].to_string(),
            target: record.target.clone(),
        });

        self.records.extend(new_records);
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.text_style.bounds.x = self.bounds.x;

        let line_height = self.text_style.line_height();
        let max_lines = (self.bounds.height / line_height) as usize;

        let bottom = if self.records.len() < max_lines {
            self.bounds.top() + line_height * self.records.len() as f32
        } else {
            self.bounds.bottom()
        };

        for (i, record) in self.records.iter().rev().enumerate() {
            if i >= max_lines {
                break;
            }

            let color = match record.level {
                LogLevel::Error => Color::new(0.9, 0.1, 0.0, 1.0),
                LogLevel::Warn => Color::new(0.9, 0.8, 0.3, 1.0),
                LogLevel::Trace | LogLevel::Debug => Color::new(0.5, 0.5, 0.5, 1.0),
                _ => Color::WHITE,
            };

            self.text_style.color = color;

            self.text_style.bounds.y = bottom - i as f32 * line_height - line_height;

            self.text_style.draw(game_io, sprite_queue, &record.message);
        }
    }
}
