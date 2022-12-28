use super::PageArrows;
use crate::render::SpriteColorQueue;
use crate::resources::{Globals, Input, InputUtil, RESOLUTION_F};
use framework::prelude::{GameIO, Vec2};

const SCROLL_SPEED: f32 = 16.0;

pub struct PageTracker {
    active_page: usize,
    page_count: usize,
    page_width: f32,
    current_offset: f32,
    page_arrows: PageArrows,
    page_arrow_positions: Vec<Vec2>,
}

impl PageTracker {
    pub fn new(game_io: &GameIO<Globals>, page_count: usize) -> Self {
        Self {
            active_page: 0,
            page_count,
            page_width: RESOLUTION_F.x,
            current_offset: 0.0,
            page_arrows: PageArrows::new(game_io, Vec2::ZERO),
            page_arrow_positions: (1..=page_count)
                .into_iter()
                .map(|i| Vec2::new(RESOLUTION_F.x * i as f32, RESOLUTION_F.y * 0.5))
                .collect(),
        }
    }

    pub fn with_page_count(mut self, width: f32) -> Self {
        self.page_width = width;
        self
    }

    pub fn active_page(&self) -> usize {
        self.active_page
    }

    pub fn page_offset(&self, page: usize) -> f32 {
        self.page_width * page as f32 - self.current_offset
    }

    pub fn animating(&self) -> bool {
        let p = self.current_offset / self.page_width;
        p.floor() != p.ceil()
    }

    pub fn update(&mut self) {
        self.page_arrows.update();

        let target_offset = self.active_page as f32 * self.page_width;

        // move towards the target
        if target_offset > self.current_offset {
            self.current_offset += SCROLL_SPEED;
        } else if target_offset < self.current_offset {
            self.current_offset -= SCROLL_SPEED;
        }

        // snap to the target value if we're close enough
        if (self.current_offset - target_offset).abs() < SCROLL_SPEED {
            self.current_offset = target_offset;
        }
    }

    pub fn handle_input(&mut self, game_io: &GameIO<Globals>) {
        let input_util = InputUtil::new(game_io);
        let original_page = self.active_page;

        let f = input_util.as_axis(Input::Left, Input::Right);

        if f < 0.0 && self.active_page > 0 {
            self.active_page -= 1;
        } else if f > 0.0 && self.active_page + 1 < self.page_count {
            self.active_page += 1;
        }

        if self.active_page != original_page {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.page_turn_sfx);
        }
    }

    pub fn visible_pages(&self) -> Vec<(usize, f32)> {
        let mut result = Vec::with_capacity(2);
        let p = self.current_offset / self.page_width;

        // calculate page a
        let page_a = p as usize;
        let page_a_offset = (p.floor() - p) * self.page_width;

        result.push((page_a, page_a_offset));

        // find out if page b is visible
        let page_b = p.ceil() as usize;

        if page_a != page_b {
            let page_b_offset = (p.ceil() - p) * self.page_width;
            result.push((page_b, page_b_offset));
        }

        result
    }

    pub fn draw_page_arrows(&mut self, sprite_queue: &mut SpriteColorQueue) {
        let mut first_run = true;

        for (page, _) in self.visible_pages() {
            if first_run && page > 0 {
                let position = self.page_arrow_positions[page - 1];
                self.page_arrows.set_position(position);
                self.page_arrows.draw(sprite_queue);
            }

            let position = self.page_arrow_positions[page];
            self.page_arrows.set_position(position);
            self.page_arrows.draw(sprite_queue);

            first_run = false;
        }
    }
}
