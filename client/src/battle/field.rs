use super::{
    BattleCallback, BattleSimulation, SharedBattleResources, TileState, TileStateAnimationSupport,
};
use super::{Entity, Tile};
use crate::bindable::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

const FRAME_ANIMATION_SUPPORT: TileStateAnimationSupport = TileStateAnimationSupport::TeamRows;

#[derive(Clone)]
pub struct Field {
    rows: usize,
    cols: usize,
    tiles: Vec<Tile>,
    tile_size: Vec2,
    frame_sprite: Sprite,
    frame_animator: Animator,
    frame_full_animator: Animator,
    time: FrameTime,
}

impl Field {
    pub fn new(game_io: &GameIO, cols: usize, rows: usize) -> Self {
        let mut tiles = Vec::with_capacity(cols * rows);

        for row in 0..rows as i32 {
            for col in 0..cols as i32 {
                let position = (col, row);
                let immutable_team = col <= 1 || col + 2 >= cols as i32;

                tiles.push(Tile::new(position, immutable_team));
            }
        }

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut frame_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_TILES);
        frame_sprite.set_color(Color::BLACK);

        Self {
            cols,
            rows,
            tiles,
            tile_size: Vec2::new(40.0, 24.0), // todo: read from .animation?
            frame_sprite,
            frame_animator: Animator::load_new(assets, ResourcePaths::BATTLE_TILE_HOLE_ANIMATION),
            frame_full_animator: Animator::load_new(
                assets,
                ResourcePaths::BATTLE_TILE_NORMAL_ANIMATION,
            ),
            time: 0,
        }
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

    pub fn drop_entity(&mut self, id: EntityId) {
        for tile in &mut self.tiles {
            tile.clear_reservations_for(id);
        }
    }

    pub fn resolve_ignored_attackers(&mut self) {
        for tile in &mut self.tiles {
            tile.unignore_inactive_attackers();
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
                    tile.set_team(team, None);
                }

                if !matches!(tile.direction(), Direction::Left | Direction::Right) {
                    tile.set_direction(direction);
                }

                if col == 0 || col == self.cols - 1 || row == 0 || row == self.rows - 1 {
                    tile.set_state_index(TileState::HIDDEN, None);
                }
            }
        }
    }

    pub fn reset_highlight(&mut self) {
        for tile in &mut self.tiles {
            tile.reset_highlight();
        }
    }

    pub fn update_tiles(&mut self, entities: &mut hecs::World) {
        for tile in &mut self.tiles {
            tile.update_state();
        }

        self.sync_team_timers(entities);
    }

    fn sync_team_timers(&mut self, entities: &mut hecs::World) {
        for col in 0..self.cols as i32 {
            let mut col_timer = FrameTime::MAX;
            let mut revert_blocked = false;

            // find the smallest time
            'rows: for row in 0..self.rows as i32 {
                let tile = self.tile_at_mut((col, row)).unwrap();

                let timer = tile.team_revert_timer();

                if col_timer > timer && timer > 0 {
                    col_timer = timer;
                }

                // test reservation blocking
                let original_team = tile.original_team();

                for id in tile.reservations().iter().cloned() {
                    if let Ok(entity) = entities.query_one_mut::<&Entity>(id.into()) {
                        // only opponents can block team changes
                        // checking for Team::Other prevents field obstacles such as rocks from blocking
                        if entity.team != original_team && entity.team != Team::Other {
                            revert_blocked = true;
                            continue 'rows;
                        }
                    }
                }

                // test unresolved neighbor blocking

                let neighbor_col = if tile.original_direction() == Direction::Right {
                    col - 1
                } else {
                    col + 1
                };

                if let Some(neighbor_tile) = self.tile_at_mut((neighbor_col, row)) {
                    // test if the neighbor must revert too
                    revert_blocked |= neighbor_tile.original_team() == original_team
                        && neighbor_tile.team() != original_team;
                }
            }

            // sync the timers if one was found
            if col_timer < FrameTime::MAX {
                if col_timer > 1 || !revert_blocked {
                    // prevent the timer from hitting 0 if there's a rule blocking us
                    col_timer -= 1;
                }

                for row in 0..self.rows as i32 {
                    let tile = self.tile_at_mut((col, row)).unwrap();
                    tile.sync_team_revert_timer(col_timer);
                }
            }
        }
    }

    pub fn apply_side_effects(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        // per tile update
        for tile in &mut simulation.field.tiles {
            let tile_state = &simulation.tile_states[tile.state_index()];
            let tile_callback = tile_state.update_callback.clone();
            let tile_position = tile.position();

            let callback = BattleCallback::new(move |game_io, resources, simulation, ()| {
                tile_callback.call(game_io, resources, simulation, tile_position);
            });

            simulation.pending_callbacks.push(callback);
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    pub fn update_animations(&mut self) {
        self.time += 1;

        for tile in &mut self.tiles {
            tile.update_team_flicker();
        }
    }

    pub fn draw(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        tile_states: &mut [TileState],
        flipped: bool,
    ) {
        sprite_queue.set_color_mode(SpriteColorMode::Add);

        let sprite_origin = Vec2::new(self.tile_size.x * 0.5, 0.0);
        let x_start = self.cols as f32 * 0.5 * -self.tile_size.x + sprite_origin.x;
        let y_start = -self.tile_size.y;

        let flip_multiplier = if flipped { -1.0 } else { 1.0 };

        let mut highlight_positions = Vec::new();

        for row in 0..self.rows {
            let state_row = (row) * 3 / (self.rows - 1) + 1;

            for col in 0..self.cols {
                let tile = &self.tiles[row * self.cols + col];
                let state_index = tile.state_index();

                if state_index == TileState::HIDDEN {
                    continue;
                }

                // resolve displayed tile state
                let tile_state = if tile.flicker_normal_state() {
                    &mut tile_states[TileState::NORMAL]
                } else {
                    &mut tile_states[state_index]
                };

                let team = tile.visible_team();

                let frame_animation_state =
                    FRAME_ANIMATION_SUPPORT.animation_state(team, state_row, flipped);

                // resolve position
                let position = Vec2::new(
                    (x_start + col as f32 * self.tile_size.x) * flip_multiplier,
                    y_start + row as f32 * self.tile_size.y,
                );

                // draw frame
                if !tile_state.hide_frame {
                    let frame_animator = if tile_state.hide_body {
                        // render just the frame
                        &mut self.frame_animator
                    } else {
                        // render the full tile
                        &mut self.frame_full_animator
                    };

                    frame_animator.set_state(frame_animation_state);
                    frame_animator.set_loop_mode(AnimatorLoopMode::Loop);
                    frame_animator.sync_time(self.time);

                    // set frame and draw
                    self.frame_sprite.set_position(position - sprite_origin);
                    frame_animator.apply(&mut self.frame_sprite);
                    sprite_queue.draw_sprite(&self.frame_sprite);
                }

                // resolve highlight
                if tile.should_highlight() {
                    highlight_positions.push(position);
                }

                // render state sprite
                let state_animation_support = tile_state.animation_support;

                if state_animation_support == TileStateAnimationSupport::None {
                    // no animation support, we can skip
                    continue;
                }

                let animation_state =
                    state_animation_support.animation_state(team, state_row, flipped);

                if tile_state.animator.set_state(animation_state) {
                    tile_state.animator.set_loop_mode(AnimatorLoopMode::Loop);
                    tile_state.animator.sync_time(self.time);

                    let mut state_sprite = Sprite::new(game_io, tile_state.texture.clone());
                    tile_state.animator.apply(&mut state_sprite);
                    state_sprite.set_position(position - sprite_origin);
                    state_sprite.set_color(Color::BLACK);
                    sprite_queue.draw_sprite(&state_sprite);
                }
            }
        }

        // draw tile highlight separately to reduce switching shaders
        sprite_queue.set_color_mode(SpriteColorMode::Multiply);

        let assets = &game_io.resource::<Globals>().unwrap().assets;
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
