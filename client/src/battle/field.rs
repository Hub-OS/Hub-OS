use super::Tile;
use crate::bindable::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

#[derive(Clone)]
pub struct Field {
    rows: usize,
    cols: usize,
    tiles: Vec<Tile>,
    tile_size: Vec2,
    red_tile_sprite: Sprite,
    blue_tile_sprite: Sprite,
    other_tile_sprite: Sprite,
    tile_animator: Animator,
    time: FrameTime,
}

impl Field {
    pub fn new(game_io: &GameIO<Globals>, cols: usize, rows: usize) -> Self {
        let mut tiles = Vec::new();
        tiles.resize_with(cols * rows, Tile::new);

        let globals = game_io.globals();
        let assets = &globals.assets;

        let mut red_tile_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_RED_TILES);
        let mut blue_tile_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_BLUE_TILES);
        let mut other_tile_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_OTHER_TILES);

        red_tile_sprite.set_color(Color::BLACK);
        blue_tile_sprite.set_color(Color::BLACK);
        other_tile_sprite.set_color(Color::BLACK);

        Self {
            cols,
            rows,
            tiles,
            tile_size: Vec2::new(40.0, 25.0), // todo: read from .animation?
            red_tile_sprite,
            blue_tile_sprite,
            other_tile_sprite,
            tile_animator: Animator::load_new(assets, ResourcePaths::BATTLE_TILE_ANIMATION),
            time: 0,
        }
    }

    pub fn set_sprites(
        &mut self,
        game_io: &GameIO<Globals>,
        red_texture_path: &str,
        blue_texture_path: &str,
        other_texture_path: &str,
        animation_path: &str,
        spacing: Vec2,
    ) {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let red_tile_texture = assets.texture(game_io, red_texture_path);
        let blue_tile_texture = assets.texture(game_io, blue_texture_path);
        let other_tile_texture = assets.texture(game_io, other_texture_path);

        self.red_tile_sprite.set_texture(red_tile_texture);
        self.blue_tile_sprite.set_texture(blue_tile_texture);
        self.other_tile_sprite.set_texture(other_tile_texture);

        self.red_tile_sprite.set_color(Color::BLACK);
        self.blue_tile_sprite.set_color(Color::BLACK);
        self.other_tile_sprite.set_color(Color::BLACK);

        self.tile_animator = Animator::load_new(assets, animation_path);
        self.tile_size = spacing;
    }

    pub fn cols(&self) -> usize {
        self.cols
    }

    pub fn rows(&self) -> usize {
        self.rows
    }

    pub fn tile_size(&self) -> Vec2 {
        self.tile_size
    }

    pub fn in_bounds(&self, (col, row): (i32, i32)) -> bool {
        col >= 0 && row >= 0 && col < self.cols as i32 && row < self.rows as i32
    }

    pub fn is_edge(&self, (col, row): (i32, i32)) -> bool {
        col == 0 || row == 0 || col + 1 == self.cols as i32 || row + 1 == self.rows as i32
    }

    pub fn tile_at_mut(&mut self, (col, row): (i32, i32)) -> Option<&mut Tile> {
        if col < 0 || row < 0 {
            return None;
        }

        let (col, row) = (col as usize, row as usize);

        self.tiles.get_mut(row * self.cols + col)
    }

    pub fn iter_mut(&mut self) -> impl Iterator<Item = ((usize, usize), &mut Tile)> {
        self.tiles.iter_mut().enumerate().map(|(index, tile)| {
            let row = index / self.cols;
            let col = index % self.cols;

            ((col, row), tile)
        })
    }

    pub fn calc_tile_center(&self, (col, row): (i32, i32), flip: bool) -> Vec2 {
        let start = Vec2::new(
            self.cols as f32 * 0.5 * -self.tile_size.x,
            -self.tile_size.y,
        );

        let mut center =
            start + self.tile_size * 0.5 + Vec2::new(col as f32, row as f32) * self.tile_size;

        if flip {
            center.x *= -1.0
        };

        center
    }

    pub fn resolve_wash(&mut self) {
        for tile in &mut self.tiles {
            tile.apply_wash();
        }
    }

    pub fn initialize_uninitialized(&mut self) {
        for row in 0..self.rows {
            for col in 0..self.cols {
                let tile = &mut self.tiles[row * self.cols + col];

                let team;
                let direction;

                if col < self.cols / 2 {
                    team = Team::Red;
                    direction = Direction::Right;
                } else {
                    team = Team::Blue;
                    direction = Direction::Left;
                }

                if tile.team() == Team::Unset {
                    tile.set_team(team);
                }

                if !matches!(tile.direction(), Direction::Left | Direction::Right) {
                    tile.set_direction(direction);
                }

                if col == 0 || col == self.cols - 1 || row == 0 || row == self.rows - 1 {
                    tile.set_state(TileState::Hidden);
                }
            }
        }
    }

    pub fn update(&mut self) {
        for tile in &mut self.tiles {
            tile.reset_highlight();
        }
    }

    pub fn update_animations(&mut self) {
        self.time += 1;
    }

    pub fn draw(
        &mut self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        flipped: bool,
    ) {
        sprite_queue.set_color_mode(SpriteColorMode::Add);

        self.tile_animator.sync_time(self.time);

        let sprite_origin = Vec2::new(self.tile_size.x * 0.5, 0.0);
        let x_start = self.cols as f32 * 0.5 * -self.tile_size.x + sprite_origin.x;
        let y_start = -self.tile_size.y;

        let red_sprite;
        let blue_sprite;

        if flipped {
            red_sprite = &mut self.blue_tile_sprite;
            blue_sprite = &mut self.red_tile_sprite;
        } else {
            red_sprite = &mut self.red_tile_sprite;
            blue_sprite = &mut self.blue_tile_sprite;
        }

        let flip_multiplier = if flipped { -1.0 } else { 1.0 };

        let mut highlight_positions = Vec::new();

        for row in 0..self.rows {
            let prefix = format!("row_{}_", 3 - (row) * 3 / (self.rows - 1));

            for col in 0..self.cols {
                let tile = &self.tiles[row * self.cols + col];

                if tile.state() == TileState::Hidden {
                    continue;
                }

                let animation_string = prefix.clone() + tile.state().animation_suffix(flipped);
                self.tile_animator.set_state(&animation_string);
                self.tile_animator.set_loop_mode(AnimatorLoopMode::Loop);
                self.tile_animator.sync_time(self.time);

                let sprite = match tile.team() {
                    Team::Red => &mut *red_sprite,
                    Team::Blue => &mut *blue_sprite,
                    _ => &mut self.other_tile_sprite,
                };

                // set position
                sprite.set_position(Vec2::new(
                    (x_start + col as f32 * self.tile_size.x) * flip_multiplier,
                    y_start + row as f32 * self.tile_size.y,
                ));

                // set frame and draw
                self.tile_animator.apply(sprite);
                sprite.set_origin(sprite_origin);
                sprite_queue.draw_sprite(sprite);

                // resolve highlight
                if tile.should_highlight() {
                    highlight_positions.push(sprite.position());
                }
            }
        }

        // draw tile highlight separately to reduce switching shaders
        sprite_queue.set_color_mode(SpriteColorMode::Multiply);

        let assets = &game_io.globals().assets;
        let mut highlight_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        highlight_sprite.set_color(Color::YELLOW);
        highlight_sprite.set_size(self.tile_size);
        highlight_sprite.set_origin(Vec2::new(0.5, 0.0));

        for position in highlight_positions {
            highlight_sprite.set_position(position);
            sprite_queue.draw_sprite(&highlight_sprite);
        }
    }
}
