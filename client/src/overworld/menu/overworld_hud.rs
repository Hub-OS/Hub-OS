use crate::overworld::{Map, OverworldArea};
use crate::render::ui::{draw_clock, FontStyle, PlayerHealthUi, Text};
use crate::render::SpriteColorQueue;
use crate::resources::{RESOLUTION_F, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;

pub struct OverworldHud {
    visible: bool,
    map_name_visible: bool,
    health_ui: PlayerHealthUi,
}

impl OverworldHud {
    pub fn new(game_io: &GameIO, health: i32) -> Self {
        Self {
            visible: true,
            map_name_visible: true,
            health_ui: PlayerHealthUi::new(game_io)
                .with_max_health(health)
                .with_health(health),
        }
    }

    pub fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }

    pub fn set_map_name_visible(&mut self, visible: bool) {
        self.map_name_visible = visible;
    }

    pub fn update(&mut self, area: &OverworldArea) {
        self.health_ui.set_health(area.player_data.health);
        self.health_ui.set_max_health(area.player_data.max_health());
        self.health_ui.update();
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, map: &Map) {
        if !self.visible {
            return;
        }

        self.health_ui.draw(game_io, sprite_queue);
        draw_clock(game_io, sprite_queue);

        if self.map_name_visible {
            draw_map_name(game_io, sprite_queue, map);
        }
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
