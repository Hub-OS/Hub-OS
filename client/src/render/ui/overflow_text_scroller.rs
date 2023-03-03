use std::ops::Range;

use crate::render::FrameTime;

const MAX_TEXT_WIDTH: usize = 8;
const FRAMES_PER_CHAR: FrameTime = 20;
const END_CHAR_LINGER: usize = 1;

pub struct OverflowTextScroller {
    time: FrameTime,
}

impl OverflowTextScroller {
    pub fn new() -> Self {
        Self { time: 0 }
    }

    pub fn reset(&mut self) {
        self.time = 0;
    }

    pub fn update(&mut self) {
        self.time += 1;
    }

    pub fn text_range(&self, text: &str) -> Range<usize> {
        let len = text.len();
        let max_end = len.max(MAX_TEXT_WIDTH) - MAX_TEXT_WIDTH;

        let index_offset = if max_end == 0 {
            0
        } else {
            let offset =
                (self.time / FRAMES_PER_CHAR) as usize % (max_end + END_CHAR_LINGER * 2 + 1);

            if offset < END_CHAR_LINGER {
                0
            } else if offset > max_end {
                // lingering
                max_end
            } else {
                offset - END_CHAR_LINGER
            }
        };

        index_offset..len.min(index_offset + MAX_TEXT_WIDTH)
    }
}
