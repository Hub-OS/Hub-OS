use super::Menu;
use crate::overworld::components::{Excluded, PlayerMapMarker};
use crate::overworld::{Map, ObjectData, ObjectType, OverworldArea, Tile, TileClass};
use crate::render::ui::Textbox;
use crate::render::{
    Animator, Background, Camera, FrameTime, MapSpriteQueue, MapTileSpriteQueue, MapTileUniforms,
    SpriteColorQueue,
};
use crate::resources::{AssetManager, Globals, Input, InputUtil, RESOLUTION_F, ResourcePaths};
use framework::{prelude::*, wgpu};
use packets::structures::Direction;

const CONCEALED_MULTIPLIER: f32 = 0.65; // 65% transparency, similar to world sprite darkening

const LAYER_START_COLOR: Color = Color::new(0.0, 0.72, 0.97, 1.0);
const LAYER_MAX_COLOR: Color = Color::new(0.03, 0.82, 0.97, 1.0);
const STAIR_COLOR: Color = Color::new(0.54, 0.99, 1.0, 1.0);
const EDGE_COLOR: Color = Color::new(0.0, 0.6, 0.91, 1.0);

const TARGET_TILE_HEIGHT: f32 = 6.0;

pub struct MapMenu {
    background: Background,
    marker_animator: Animator,
    marker_sprite: Sprite,
    tile_marker_sprites: Vec<Sprite>,
    overlay_sprite: Sprite,
    overlay_arrows_sprite: Sprite,
    map_sprite: Sprite,
    tile_target: RenderTarget,
    edge_target: RenderTarget,
    scale: f32,
    scrollable: bool,
    scroll_offset: Vec2,
    last_map_update: FrameTime,
    open: bool,
}

impl MapMenu {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        Self {
            background: Background::new(
                Animator::load_new(assets, ResourcePaths::OVERWORLD_MAP_BG_ANIMATION),
                assets.new_sprite(game_io, ResourcePaths::OVERWORLD_MAP_BG),
            ),
            marker_animator: Animator::load_new(
                assets,
                ResourcePaths::OVERWORLD_MAP_MARKERS_ANIMATION,
            ),
            marker_sprite: assets.new_sprite(game_io, ResourcePaths::OVERWORLD_MAP_MARKERS),
            tile_marker_sprites: Vec::new(),
            overlay_sprite: assets.new_sprite(game_io, ResourcePaths::OVERWORLD_MAP_OVERLAY),
            overlay_arrows_sprite: assets
                .new_sprite(game_io, ResourcePaths::OVERWORLD_MAP_OVERLAY_ARROWS),
            map_sprite: assets.new_sprite(game_io, ResourcePaths::BLANK),
            tile_target: RenderTarget::new(game_io, UVec2::ONE),
            edge_target: RenderTarget::new(game_io, UVec2::ONE),
            scale: 1.0,
            scrollable: false,
            scroll_offset: Vec2::ZERO,
            last_map_update: -1,
            open: false,
        }
    }

    fn update_map_sprites(&mut self, game_io: &GameIO, map: &Map) {
        let tile_size = map.tile_size().as_vec2();

        self.scale = TARGET_TILE_HEIGHT / tile_size.y;
        let texture_size = Self::calculate_texture_size_and_scale(map, self.scale);

        let center = Self::calculate_map_center(map);

        // texture does not fit on screen, allow large map controls
        self.scrollable =
            texture_size.x > RESOLUTION_F.x as u32 || texture_size.y > RESOLUTION_F.y as u32;

        // create the camera
        let mut camera = Camera::new(game_io);
        camera.snap(center);

        let camera_scale_correction = RESOLUTION_F / texture_size.as_vec2();
        camera.set_scale(Vec2::new(self.scale, self.scale) * camera_scale_correction);

        let max_layer_count = map.tile_layers().len();
        let half_tile_size = tile_size * 0.5;

        // initialize targets and passes
        let device = game_io.graphics().device();
        let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
            label: Some("map_command_encoder"),
        });

        self.edge_target.resize(game_io, texture_size);
        self.tile_target.resize(game_io, texture_size);

        let mut edge_pass = RenderPass::new(&mut encoder, &self.edge_target);
        let mut tile_pass = edge_pass.create_subpass(&self.tile_target);

        // tile pass
        let mut sprite_queue = MapTileSpriteQueue::new(game_io, &camera);

        for i in 0..max_layer_count {
            let progress = i as f32 / max_layer_count as f32;
            let color = Color::lerp(LAYER_START_COLOR, LAYER_MAX_COLOR, progress);

            map.iterate_visible_tiles(game_io, &camera, i, |sprite, tile_meta, _, tile_pos| {
                if i > 0 && map.ignore_tile_above(tile_pos.into(), (i - 1) as _) {
                    return;
                }

                if !tile_meta.minimap {
                    return;
                }

                let frame = sprite.frame();

                let mut center = frame.top_left();

                center.x += half_tile_size.x;
                center.x -= tile_meta.drawing_offset.x;

                center.y += frame.height - half_tile_size.y;
                center.y -= tile_meta.drawing_offset.y;

                let point_to_uv_multiplier = 1.0 / sprite.texture().size().as_vec2();

                if tile_meta.tile_class == TileClass::Stairs {
                    sprite.set_color(STAIR_COLOR);
                } else {
                    sprite.set_color(color);
                }

                // slightly increase size of sprite to reduce holes in final render
                let size = sprite.size();
                sprite.set_size(size + Vec2::ONE);

                // todo: use instancing the same as background.rs
                sprite_queue.set_tile_uniforms(MapTileUniforms {
                    center: center * point_to_uv_multiplier,
                    tile_size: (tile_size + 1.0) * point_to_uv_multiplier,
                    trim: (tile_meta.tile_class != TileClass::Stairs) as i32,
                    padding: 0,
                });

                sprite_queue.draw_sprite(sprite);
            });
        }

        tile_pass.consume_queue(sprite_queue);
        tile_pass.flush();

        // edge pass
        let mut camera = Camera::new_ui(game_io);
        camera.snap(self.tile_target.size().as_vec2() * 0.5);
        camera.set_scale(RESOLUTION_F / texture_size.as_vec2());

        let mut sprite_queue = MapSpriteQueue::new(game_io, &camera);
        let mut sprite = Sprite::new(game_io, self.tile_target.texture().clone());
        sprite.set_color(EDGE_COLOR);
        sprite_queue.draw_sprite(&sprite);
        edge_pass.consume_queue(sprite_queue);
        edge_pass.flush();

        // submit changes
        let queue = game_io.graphics().queue();
        queue.submit([encoder.finish()]);

        // update sprite texture
        let map_texture = self.edge_target.texture().clone();
        self.map_sprite.set_texture(map_texture);

        // save origin
        let origin = Vec2::new(
            (texture_size.x / 2) as f32 - center.x * self.scale,
            (max_layer_count as f32 * tile_size.y) * 0.5 * self.scale,
        );

        self.map_sprite.set_origin(origin.ceil());

        // default position
        let default_position = (RESOLUTION_F - texture_size.as_vec2()) * 0.5 + origin;
        self.map_sprite.set_position(default_position.floor());

        // update tile markers
        self.update_tile_markers(map);
    }

    fn calculate_texture_size_and_scale(map: &Map, scale: f32) -> UVec2 {
        let map_screen_unit_dimensions = Vec2::new(
            map.world_to_screen(Vec2::new(map.cols() as f32, 0.0)).x
                - map.world_to_screen(Vec2::new(0.0, map.rows() as f32)).x,
            (map.cols() + map.rows()) as f32 + map.tile_layers().len() as f32 + 1.0,
        );

        let tile_size = map.tile_size().as_vec2();

        let texture_size_f = Vec2::new(
            (map_screen_unit_dimensions.x * tile_size.x * scale * 0.5).ceil(),
            (map_screen_unit_dimensions.y * tile_size.y * scale * 0.5).ceil(),
        );

        texture_size_f.as_uvec2()
    }

    fn calculate_map_center(map: &Map) -> Vec2 {
        let layer_dimensions = map.tile_to_world(Vec2::new(map.cols() as f32, map.rows() as f32));
        let map_dimensions = Vec3::new(
            layer_dimensions.x,
            layer_dimensions.y,
            map.tile_layers().len() as f32 - 1.0,
        );

        map.world_3d_to_screen(map_dimensions * 0.5)
    }

    fn update_tile_markers(&mut self, map: &Map) {
        let mut z = 0.0;
        let cols = map.cols() as i32;
        let rows = map.rows() as i32;

        self.tile_marker_sprites.clear();

        for layer in map.tile_layers() {
            for row in 0..rows {
                for col in 0..cols {
                    let tile_position = IVec2::new(col, row);
                    let tile = layer.tile_at(tile_position);
                    let Some(tile_meta) = map.tile_meta_for_tile(tile.gid) else {
                        continue;
                    };

                    let state = match tile_meta.tile_class {
                        TileClass::Conveyor => "CONVEYOR",
                        TileClass::Ice => "ICE",
                        TileClass::Treadmill => "TREADMILL",
                        TileClass::Arrow => "ARROW",
                        _ => {
                            continue;
                        }
                    };

                    // offset by 0.5 to get the center position
                    let tile_position = tile_position.as_vec2() + Vec2::new(0.5, 0.5);
                    let world_position = map.tile_to_world(tile_position).extend(z);
                    let screen_position = map.world_3d_to_screen(world_position) * self.scale;

                    let mut sprite = self.marker_sprite.clone();

                    self.marker_animator.set_state(state);
                    self.marker_animator.apply(&mut sprite);

                    if map.is_concealed(world_position) {
                        sprite.set_color(Color::WHITE.multiply_color(CONCEALED_MULTIPLIER));
                    } else {
                        sprite.set_color(Color::WHITE);
                    }

                    sprite.set_position(screen_position.floor());
                    sprite.set_scale(Self::scale_from_direction(tile.get_direction(tile_meta)));

                    self.tile_marker_sprites.push(sprite);
                }
            }

            z += 1.0;
        }
    }

    fn scale_from_direction(direction: Direction) -> Vec2 {
        match direction {
            Direction::UpLeft => Vec2::new(-1.0, -1.0),
            Direction::UpRight => Vec2::new(1.0, -1.0),
            Direction::DownLeft => Vec2::new(-1.0, 1.0),
            _ => Vec2::ONE,
        }
    }

    fn center_on_player(&mut self, area: &mut OverworldArea) {
        let entities = &mut area.entities;
        let Ok(&position) = entities.query_one_mut::<&Vec3>(area.player_data.entity) else {
            return;
        };

        let sprite_position = area.map.world_3d_to_screen(position) * self.scale;
        let map_position = -sprite_position + RESOLUTION_F * 0.5;
        self.map_sprite.set_position(map_position.floor());
    }
}

impl Menu for MapMenu {
    fn is_fullscreen(&self) -> bool {
        true
    }

    fn is_open(&self) -> bool {
        self.open
    }

    fn open(&mut self, _game_io: &mut GameIO, _area: &mut OverworldArea) {
        self.open = true;
        self.scroll_offset = Vec2::ZERO;
    }

    fn update(&mut self, game_io: &mut GameIO, area: &mut OverworldArea) {
        self.background.update();

        if self.last_map_update != area.last_map_update {
            self.update_map_sprites(game_io, &area.map);
            self.last_map_update = area.last_map_update;
        }

        if self.scrollable {
            self.center_on_player(area);
            self.map_sprite
                .set_position(self.map_sprite.position() - self.scroll_offset);
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO, _: &mut OverworldArea, _: &mut Textbox) {
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) || input_util.was_just_pressed(Input::Map) {
            self.open = false;
        }

        if !self.scrollable {
            return;
        }

        let offset_change = Vec2::new(
            input_util.as_axis(Input::Left, Input::Right),
            input_util.as_axis(Input::Up, Input::Down),
        );
        self.scroll_offset += offset_change;

        // clamping
        let current_position = self.map_sprite.position();
        let texture_size = self.map_sprite.bounds().size();

        let sprite_offset = current_position - self.map_sprite.origin() - offset_change;
        let center_distance = -sprite_offset - ((texture_size - RESOLUTION_F) * 0.5).floor();

        let view_size = texture_size * 0.5;

        // clamp -x
        if center_distance.x < -view_size.x {
            self.scroll_offset.x += -view_size.x - center_distance.x;
        }

        // clamp -y
        if center_distance.y < -view_size.y {
            self.scroll_offset.y += -view_size.y - center_distance.y;
        }

        // clamp +x
        if center_distance.x > view_size.x {
            self.scroll_offset.x += view_size.x - center_distance.x;
        }

        // clamp +y
        if center_distance.y > view_size.y {
            self.scroll_offset.y += view_size.y - center_distance.y;
        }
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        render_pass: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
        area: &OverworldArea,
    ) {
        // draw bg
        self.background.draw(game_io, render_pass);

        // draw map
        sprite_queue.draw_sprite(&self.map_sprite);

        let map_offset = self.map_sprite.position();

        // draw tile markers
        for sprite in &mut self.tile_marker_sprites {
            let original_position = sprite.position();

            sprite.set_position(original_position + map_offset);
            sprite_queue.draw_sprite(sprite);
            sprite.set_position(original_position);
        }

        // draw objects
        type ObjectQuery<'a> =
            hecs::Without<(&'a Vec3, Option<&'a Tile>, &'a ObjectData), &'a Excluded>;
        let object_entities = area.map.object_entities();

        for (_, (&position, tile, object)) in object_entities.query::<ObjectQuery>().into_iter() {
            // resolve state
            let state = match object.object_type {
                ObjectType::CustomWarp
                | ObjectType::ServerWarp
                | ObjectType::PositionWarp
                | ObjectType::CustomServerWarp => "WARP",
                ObjectType::HomeWarp => "HOME",
                ObjectType::Board => "BOARD",
                ObjectType::Shop => "SHOP",
                ObjectType::Bookmark => "BOOKMARK",
                _ => {
                    continue;
                }
            };

            // reset scale
            self.marker_sprite.set_scale(Vec2::ONE);

            self.marker_animator.set_state(state);
            self.marker_animator.apply(&mut self.marker_sprite);

            let mut screen_position = area.map.world_3d_to_screen(position);
            let mut center_offset = Vec2::ZERO;

            if let Some(tile) = tile {
                let Some(tile_meta) = area.map.tile_meta_for_tile(tile.gid) else {
                    continue;
                };

                // resolve scale
                let flip = object.object_type == ObjectType::Board && tile.flipped_horizontal;
                let marker_scale = if flip {
                    Vec2::new(-1.0, 1.0)
                } else {
                    Vec2::ONE
                };

                self.marker_sprite.set_scale(marker_scale);

                // resolve offset
                center_offset =
                    tile_meta.alignment_offset + tile_meta.drawing_offset + object.size * 0.5;
            }

            // finish resolving position
            if object.object_type.is_warp() {
                screen_position += center_offset;
            } else if object.object_type == ObjectType::Board {
                screen_position += center_offset;
                screen_position.y += object.size.y * 0.5;
            }

            self.marker_sprite
                .set_position((screen_position * self.scale + map_offset).floor());

            // resolve color
            let color = if area.map.is_concealed(position) {
                Color::WHITE.multiply_color(CONCEALED_MULTIPLIER)
            } else {
                Color::WHITE
            };

            self.marker_sprite.set_color(color);

            // draw
            sprite_queue.draw_sprite(&self.marker_sprite);
        }

        // draw actors
        type ActorQuery<'a> = hecs::Without<(&'a Vec3, &'a PlayerMapMarker), &'a Excluded>;

        self.marker_sprite.set_scale(Vec2::ONE);
        self.marker_animator.set_state("ACTOR");
        self.marker_animator.apply(&mut self.marker_sprite);

        for (entity, (&position, marker)) in area.entities.query::<ActorQuery>().into_iter() {
            // resolve position
            let mut sprite_position =
                area.map.world_3d_to_screen(position) * self.scale + map_offset;

            // bobbing to make the player's marker grab focus
            if area.player_data.entity == entity {
                sprite_position.y += ((area.world_time / 10) % 2) as f32;
            }

            self.marker_sprite.set_position(sprite_position.floor());

            // resolve color
            let color = if area.map.is_concealed(position) {
                marker.color.multiply_color(CONCEALED_MULTIPLIER)
            } else {
                marker.color
            };

            self.marker_sprite.set_color(color);

            // draw sprite
            sprite_queue.draw_sprite(&self.marker_sprite);
        }

        // draw overlay
        sprite_queue.draw_sprite(&self.overlay_sprite);

        if self.scrollable {
            sprite_queue.draw_sprite(&self.overlay_arrows_sprite);
        }
    }
}
