use crate::overworld::{Map, OverworldArea};
use crate::render::ui::{draw_clock, FontStyle, PlayerHealthUI, Text};
use crate::render::SpriteColorQueue;
use crate::resources::{RESOLUTION_F, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;

pub struct OverworldHud {
    health_ui: PlayerHealthUI,
}

impl OverworldHud {
    pub fn new(game_io: &GameIO, health: i32) -> Self {
        Self {
            health_ui: PlayerHealthUI::new(game_io).with_health(health),
        }
    }

    pub fn update(&mut self, area: &OverworldArea) {
        self.health_ui.set_health(area.player_data.health);
        self.health_ui.update();
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, map: &Map) {
        draw_clock(game_io, sprite_queue);
        draw_map_name(game_io, sprite_queue, map);
        self.health_ui.draw(game_io, sprite_queue);
    }
}

const TEXT_SHADOW_COLOR: Color = Color::new(0.41, 0.41, 0.41, 1.0);

fn draw_map_name(game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, map: &Map) {
    const MARGIN: Vec2 = Vec2::new(1.0, 3.0);

    let mut label = Text::new(game_io, FontStyle::Thick);
    label.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
    label.text = format!("{:_>12}", map.name());

    let text_size = label.measure().size;

    (label.style.bounds).set_position(RESOLUTION_F - text_size - MARGIN);

    // draw text
    label.draw(game_io, sprite_queue);
}