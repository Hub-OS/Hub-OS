use super::Tile;
use super::{
    BattleCallback, BattleSimulation, SharedBattleResources, TileState, TileStateAnimationSupport,
};
use crate::bindable::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::sync::Arc;

const FRAME_ANIMATION_SUPPORT: TileStateAnimationSupport = TileStateAnimationSupport::TeamRows;

#[derive(Clone)]
pub struct Field {
    rows: usize,
    cols: usize,
    tiles: Vec<Tile>,
    tile_size: Vec2,
    frame_sprite: Sprite,
    frame_animator: Animator,
    frame_animation_support: TileStateAnimationSupport,
    filled_sprite: Sprite,
    filled_animator: Animator,
    filled_animation_support: TileStateAnimationSupport,
    time: FrameTime,
}

impl Field {
    pub fn new(game_io: &GameIO) -> Self {
        let cols = FIELD_DEFAULT_WIDTH;
        let rows = FIELD_DEFAULT_HEIGHT;

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
        let frame_animator = Animator::load_new(assets, ResourcePaths::BATTLE_TILE_HOLE_ANIMATION);
        let frame_animation_support = TileStateAnimationSupport::from_animator(&frame_animator);

        let filled_sprite = frame_sprite.clone();
        let filled_animator =
            Animator::load_new(assets, ResourcePaths::BATTLE_TILE_NORMAL_ANIMATION);
        let filled_animation_support = TileStateAnimationSupport::from_animator(&filled_animator);

        Self {
            cols,
            rows,
            tiles,
            tile_size: Vec2::new(40.0, 24.0), // todo: read from .animation?
            frame_sprite,
            frame_animator,
            frame_animation_support,
            filled_sprite,
            filled_animator,
            filled_animation_support,
            time: 0,
        }
    }

    pub fn cols(&self) -> usize {
        self.cols
    }

    pub fn rows(&self) -> usize {
        self.rows
    }

    pub fn resize(&mut self, cols: usize, rows: usize) {
        self.tiles.clear();

        for row in 0..rows {
            for col in 0..cols {
                let position = (col as i32, row as i32);
                let immutable_team = col <= 1 || col + 2 >= cols;

                self.tiles.push(Tile::new(position, immutable_team));
            }
        }

        self.cols = cols;
        self.rows = rows;
    }

    pub fn tile_size(&self) -> Vec2 {
        self.tile_size
    }

    pub fn set_tile_size(&mut self, size: Vec2) {
        self.tile_size = size;
    }

    pub fn set_tile_frame_resources(&mut self, texture: Arc<Texture>, animator: Animator) {
        self.frame_sprite.set_texture(texture);
        self.frame_animation_support = TileStateAnimationSupport::from_animator(&animator);
        self.frame_animator = animator;
    }

    pub fn set_tile_filled_resources(&mut self, texture: Arc<Texture>, animator: Animator) {
        self.filled_sprite.set_texture(texture);
        self.filled_animation_support = TileStateAnimationSupport::from_animator(&animator);
        self.filled_animator = animator;
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
        let mut center = self.top_left()
            + self.tile_size * 0.5
            + Vec2::new(col as f32, row as f32) * self.tile_size;

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

    pub fn resolve_ignored_attackers(&mut self, entities: &mut hecs::World) {
        for tile in &mut self.tiles {
            tile.unignore_inactive_attackers(entities);
        }
    }

    pub fn initialize_uninitialized(&mut self, entities: &mut hecs::World) {
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
                    tile.set_team(entities, team, tile.direction());
                }

                if !matches!(tile.direction(), Direction::Left | Direction::Right) {
                    tile.set_direction(direction);
                }

                if col == 0 || col == self.cols - 1 || row == 0 || row == self.rows - 1 {
                    tile.set_state_index(TileState::VOID, None);
                }
            }
        }
    }

    pub fn reset_highlight(&mut self) {
        for tile in &mut self.tiles {
            tile.reset_highlight();
        }
    }

    fn sync_team_timers(&mut self, entities: &mut hecs::World, time_frozen: bool) {
        #[derive(Clone, Copy)]
        struct TeamState {
            timer: FrameTime,
            has_reservation: bool,
            behind_claimed: bool,
        }

        impl Default for TeamState {
            fn default() -> Self {
                Self {
                    timer: FrameTime::MAX,
                    has_reservation: false,
                    behind_claimed: false,
                }
            }
        }

        for col in 0..self.cols as i32 {
            let mut team_states = [TeamState::default(); 4];

            // resolve team states in the column
            for row in 0..self.rows as i32 {
                let tile = self.tile_at_mut((col, row)).unwrap();
                let team = tile.team();
                let team_state = &mut team_states[team as usize];

                // test reservation blocking
                team_state.has_reservation |=
                    tile.has_claim_blocking_reservation(entities, tile.original_team());

                let timer = tile.team_reclaim_timer();

                if team_state.timer > timer && team != tile.original_team() {
                    team_state.timer = timer;
                }

                // test unresolved neighbor blocking
                let direction = tile.direction();
                let neighbor_col = if direction == Direction::Right {
                    col + 1
                } else {
                    col - 1
                };

                // test if there's a claim past this column
                let original_team = tile.original_team();

                if let Some(neighbor_tile) = self.tile_at_mut((neighbor_col, row)) {
                    team_state.behind_claimed |= neighbor_tile.direction() == direction
                        && neighbor_tile.original_team() == original_team
                        && neighbor_tile.team() != original_team;
                }
            }

            // syncing timers
            for (team_i, team_state) in team_states.iter().enumerate() {
                let mut timer = team_state.timer;

                if timer == FrameTime::MAX {
                    // no timers to sync
                    continue;
                }

                if !time_frozen && timer > 0 {
                    timer -= 1;
                }

                // reclaiming is blocked if we're behind another column that we've claimed
                // or if we have a reservation in the column
                let reclaim_blocked = team_state.behind_claimed || team_state.has_reservation;

                // sync tiles in the same column that match our team
                for row in 0..self.rows as i32 {
                    let tile = self.tile_at_mut((col, row)).unwrap();

                    if tile.team() as usize != team_i {
                        continue;
                    }

                    tile.set_team_reclaim_timer(timer);

                    if !reclaim_blocked {
                        tile.try_reclaim();
                    }
                }
            }
        }
    }

    pub fn update(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let time_frozen = simulation.time_freeze_tracker.time_is_frozen();
        let entities = &mut simulation.entities;

        simulation.field.reset_highlight();
        simulation.field.sync_team_timers(entities, time_frozen);

        if time_frozen {
            return;
        }

        // revert state timers
        for tile in &mut simulation.field.tiles {
            let prev_state_index = tile.state_index();

            tile.update_state();

            if tile.state_index() == prev_state_index {
                continue;
            }

            let tile_state = &simulation.tile_states[prev_state_index];
            let tile_callback = tile_state.replace_callback.clone();
            let tile_position = tile.position();

            let callback = BattleCallback::new(move |game_io, resources, simulation, ()| {
                tile_callback.call(game_io, resources, simulation, tile_position);
            });

            simulation.pending_callbacks.push(callback);
        }

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

    pub fn top_left(&self) -> Vec2 {
        Vec2::new(
            self.cols as f32 * 0.5 * -self.tile_size.x,
            -self.tile_size.y,
        )
    }

    pub fn best_fitting_scale(&self) -> Vec2 {
        // field tile size with edges trimmed off
        let field_tile_size =
            (Vec2::new((self.cols - 2) as _, (self.rows - 2) as _)).max(Vec2::ONE);
        let field_size = self.tile_size * field_tile_size;

        let allocated_space = Vec2::new(
            RESOLUTION_F.x,
            RESOLUTION_F.y * 0.5 + BATTLE_CARD_SELECT_CAMERA_OFFSET.y,
        );

        let scale = (allocated_space / field_size).min_element();

        if scale >= 1.0 {
            Vec2::ONE
        } else {
            Vec2::splat(scale)
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
        let top_left = self.top_left() + sprite_origin;

        let flip_multiplier = if flipped { -1.0 } else { 1.0 };

        let mut highlight_positions = Vec::new();

        for row in 0..self.rows {
            let state_row = if row + 2 >= self.rows {
                3
            } else if row <= 1 {
                1
            } else {
                2
            };

            for col in 0..self.cols {
                let tile = &self.tiles[row * self.cols + col];
                let state_index = tile.visible_state_index();

                if state_index == TileState::VOID {
                    continue;
                }

                // resolve displayed tile state
                let tile_state = if tile.flicker_normal_state() {
                    &mut tile_states[TileState::NORMAL]
                } else {
                    &mut tile_states[state_index]
                };

                let team = tile.visible_team();

                // resolve position
                let position = Vec2::new(
                    (top_left.x + col as f32 * self.tile_size.x) * flip_multiplier,
                    top_left.y + row as f32 * self.tile_size.y,
                );

                // draw frame
                if !tile_state.hide_frame {
                    let (animator, sprite, animation_support) = if tile_state.hide_body {
                        // render just the frame
                        (
                            &mut self.frame_animator,
                            &mut self.frame_sprite,
                            self.frame_animation_support,
                        )
                    } else {
                        // render the full tile
                        (
                            &mut self.filled_animator,
                            &mut self.filled_sprite,
                            self.filled_animation_support,
                        )
                    };

                    let frame_animation_state =
                        animation_support.animation_state(team, state_row, flipped);

                    animator.set_state(frame_animation_state);
                    animator.set_loop_mode(AnimatorLoopMode::Loop);
                    animator.sync_time(self.time);

                    // set frame and draw
                    sprite.set_position(position - sprite_origin);
                    animator.apply(sprite);
                    sprite_queue.draw_sprite(sprite);
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
        let mut highlight_sprite = assets.new_sprite(game_io, ResourcePaths::PIXEL);
        highlight_sprite.set_color(Color::YELLOW);
        highlight_sprite.set_size(self.tile_size);
        highlight_sprite.set_origin(Vec2::new(0.5, 0.0));

        for position in highlight_positions {
            highlight_sprite.set_position(position);
            sprite_queue.draw_sprite(&highlight_sprite);
        }
    }
}
