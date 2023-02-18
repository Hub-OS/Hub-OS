use super::components::*;
use super::*;
use crate::render::*;
use framework::prelude::*;
use hecs::Entity;
use std::collections::HashMap;
use std::rc::Rc;

pub struct Map {
    cols: u32,
    rows: u32,
    tile_size: IVec2,
    name: String,
    song_path: String,
    background_properties: BackgroundProperties,
    foreground_properties: BackgroundProperties,
    shadow_map: ShadowMap,
    tile_layers: Vec<TileLayer>,
    tile_to_tileset: Vec<usize>,
    tilesets: Vec<Rc<Tileset>>,
    tile_metas: Vec<Option<TileMeta>>,
    tiles_modified: bool,
    object_entity_map: HashMap<u32, hecs::Entity>,
    object_entities: hecs::World,
    projection: Projection,
}

impl Map {
    pub fn new(cols: u32, rows: u32, tile_width: i32, tile_height: i32) -> Self {
        Self {
            cols,
            rows,
            tile_size: IVec2::new(tile_width, tile_height),
            name: String::new(),
            song_path: String::new(),
            background_properties: BackgroundProperties::default(),
            foreground_properties: BackgroundProperties::default(),
            shadow_map: ShadowMap::new(cols as usize, rows as usize),
            tile_layers: Vec::new(),
            tile_to_tileset: Vec::new(),
            tilesets: Vec::new(),
            tile_metas: Vec::new(),
            tiles_modified: false,
            object_entity_map: HashMap::new(),
            object_entities: hecs::World::new(),
            projection: Projection::Isometric,
        }
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn set_name(&mut self, name: String) {
        self.name = name;
    }

    pub fn background_properties(&self) -> &BackgroundProperties {
        &self.background_properties
    }

    pub fn background_properties_mut(&mut self) -> &mut BackgroundProperties {
        &mut self.background_properties
    }

    pub fn set_background_properties(&mut self, background_properties: BackgroundProperties) {
        self.background_properties = background_properties;
    }

    pub fn foreground_properties(&self) -> &BackgroundProperties {
        &self.foreground_properties
    }

    pub fn foreground_properties_mut(&mut self) -> &mut BackgroundProperties {
        &mut self.foreground_properties
    }

    pub fn set_foreground_properties(&mut self, foreground_properties: BackgroundProperties) {
        self.foreground_properties = foreground_properties;
    }

    pub fn song_path(&self) -> &str {
        &self.song_path
    }

    pub fn set_song_path(&mut self, path: String) {
        self.song_path = path;
    }

    pub fn cols(&self) -> u32 {
        self.cols
    }

    pub fn rows(&self) -> u32 {
        self.rows
    }

    pub fn tile_size(&self) -> IVec2 {
        self.tile_size
    }

    pub fn tile_meta_for_tile(&self, tile_gid: u32) -> Option<&TileMeta> {
        self.tile_metas
            .get(tile_gid as usize)
            .and_then(|o| o.as_ref())
    }

    pub fn tilesets(&self) -> &[Rc<Tileset>] {
        &self.tilesets
    }

    pub fn add_tile_meta(&mut self, tile_meta: TileMeta) {
        let has_tileset = self
            .tilesets
            .iter()
            .any(|tileset| Rc::ptr_eq(tileset, &tile_meta.tileset));

        if !has_tileset {
            self.tilesets.push(tile_meta.tileset.clone());
        }

        let index = tile_meta.gid as usize;
        let expected_size = index + 1;

        if self.tile_metas.len() < expected_size {
            self.tile_metas.resize_with(expected_size, || None);
        }

        self.tile_metas[index] = Some(tile_meta);
        self.tiles_modified = true;
    }

    pub fn tile_layers(&self) -> &[TileLayer] {
        &self.tile_layers
    }

    pub fn tile_layers_mut(&mut self) -> &mut [TileLayer] {
        &mut self.tile_layers
    }

    pub fn tile_layer(&self, index: usize) -> Option<&TileLayer> {
        self.tile_layers.get(index)
    }

    pub fn tile_layer_mut(&mut self, index: usize) -> Option<&mut TileLayer> {
        self.tile_layers.get_mut(index)
    }

    pub fn add_layer(&mut self) -> &mut TileLayer {
        self.tile_layers.push(TileLayer::new(self.cols, self.rows));
        self.tile_layers.last_mut().unwrap()
    }

    pub fn object_entities(&self) -> &hecs::World {
        &self.object_entities
    }

    pub fn object_entities_mut(&mut self) -> &mut hecs::World {
        &mut self.object_entities
    }

    pub fn insert_tile_object(
        &mut self,
        game_io: &GameIO,
        tile_object: TileObject,
        layer_index: usize,
    ) {
        let id = tile_object.data.id;
        let position = tile_object.data.position;
        let tile_position = self.world_to_tile_space(position);
        let elevation = self.elevation_at(tile_position, layer_index as i32);
        let sprite = tile_object.create_sprite(game_io, self, layer_index);

        let mut entity = hecs::EntityBuilder::new();

        entity.add_bundle((
            tile_object.data,
            tile_object.tile,
            position.extend(elevation),
        ));

        if let Some(sprite) = sprite {
            entity.add(sprite);
        }

        let entity = self.object_entities.spawn(entity.build());
        self.object_entity_map.insert(id, entity);
    }

    pub fn insert_shape_object(&mut self, shape_object: ShapeObject) {
        let id = shape_object.data.id;

        let entity = self
            .object_entities
            .spawn((shape_object.data, shape_object.shape));

        self.object_entity_map.insert(id, entity);
    }

    pub fn get_object_entity(&self, id: u32) -> Option<hecs::Entity> {
        self.object_entity_map.get(&id).cloned()
    }

    pub fn screen_direction_to_world(&self, direction: Direction) -> Direction {
        if self.projection == Projection::Orthographic {
            direction
        } else {
            match direction {
                Direction::Up => Direction::UpLeft,
                Direction::Left => Direction::DownLeft,
                Direction::Down => Direction::DownRight,
                Direction::Right => Direction::UpRight,
                Direction::UpLeft => Direction::Left,
                Direction::UpRight => Direction::Up,
                Direction::DownLeft => Direction::Down,
                Direction::DownRight => Direction::Right,
                Direction::None => Direction::None,
            }
        }
    }

    pub fn world_direction_to_screen(&self, direction: Direction) -> Direction {
        if self.projection == Projection::Orthographic {
            direction
        } else {
            match direction {
                Direction::Up => Direction::UpRight,
                Direction::Left => Direction::UpLeft,
                Direction::Down => Direction::DownLeft,
                Direction::Right => Direction::DownRight,
                Direction::UpLeft => Direction::Up,
                Direction::UpRight => Direction::Right,
                Direction::DownLeft => Direction::Left,
                Direction::DownRight => Direction::Down,
                Direction::None => Direction::None,
            }
        }
    }

    pub fn screen_to_world(&self, point: Vec2) -> Vec2 {
        Vec2::new(
            (2.0 * point.y + point.x) * 0.5,
            (2.0 * point.y - point.x) * 0.5,
        )
    }

    pub fn world_to_screen(&self, point: Vec2) -> Vec2 {
        Vec2::new(point.x - point.y, (point.x + point.y) * 0.5)
    }

    pub fn world_3d_to_screen(&self, point: Vec3) -> Vec2 {
        let mut screen_point = self.world_to_screen(point.xy());
        screen_point.y -= point.z * self.tile_size.y as f32 * 0.5;

        screen_point
    }

    pub fn world_to_tile_space(&self, point: Vec2) -> Vec2 {
        Vec2::new(
            point.x / (self.tile_size.x as f32) * 2.0,
            point.y / (self.tile_size.y as f32),
        )
    }

    pub fn world_3d_to_tile_space(&self, point: Vec3) -> Vec3 {
        self.world_to_tile_space(point.xy()).extend(point.z)
    }

    pub fn tile_3d_to_world(&self, tile: Vec3) -> Vec3 {
        let mut world = tile * self.tile_size.as_vec2().extend(1.0);
        world.x /= 2.0;
        world
    }

    pub fn can_move_to(&self, pos: Vec3) -> bool {
        let layer_index = pos.z as i32;

        if layer_index < 0 || layer_index >= self.tile_layers.len() as i32 {
            return false;
        }

        let layer = self.tile_layer(layer_index as usize).unwrap();
        let tile = layer.tile_at_f32(pos.xy());

        if tile.gid == 0 {
            return false;
        }

        // get decimal part
        let mut test_pos = pos.xy().fract();

        // get positive coords
        if test_pos.x < 0.0 {
            test_pos.x += 1.0;
        }
        if test_pos.y < 0.0 {
            test_pos.y += 1.0;
        }

        // convert to iso pixels
        test_pos.x *= (self.tile_size.x / 2) as f32;
        test_pos.y *= self.tile_size.y as f32;

        let layer_relative_z = pos.z.fract();
        let tile_test_pos = test_pos - layer_relative_z * self.tile_size.y as f32;

        if tile.intersects(self, tile_test_pos) {
            return false;
        }

        test_pos.x += pos.x.floor() * (self.tile_size.x / 2) as f32;
        test_pos.y += pos.y.floor() * (self.tile_size.y) as f32;

        self.tile_object_at(test_pos, layer_index, true).is_none()
    }

    pub fn elevation_at(&self, point: Vec2, mut layer_index: i32) -> f32 {
        let total_layers = self.tile_layers.len() as i32;

        if layer_index >= total_layers {
            layer_index = total_layers - 1;
        }

        layer_index = layer_index.max(0);
        let layer_elevation = layer_index as f32;

        let layer = match self.tile_layers.get(layer_index as usize) {
            Some(layer) => layer,
            None => return 0.0,
        };

        let tile = layer.tile_at_f32(point);
        let tile_meta = match self.tile_meta_for_tile(tile.gid) {
            Some(tile_meta) => tile_meta,
            _ => return layer_elevation,
        };

        if tile_meta.tile_class != TileClass::Stairs {
            return layer_elevation;
        }

        let relative_x = point.x.fract();
        let relative_y = point.y.fract();

        let mut direction: Direction = tile_meta.custom_properties.get("direction").into();

        if tile.flipped_horizontal {
            direction = direction.horizontal_mirror();
        }

        if tile.flipped_vertical {
            direction = direction.vertical_mirror();
        }

        let layer_relative_elevation = match direction {
            Direction::UpLeft => 1.0 - relative_x,
            Direction::UpRight => 1.0 - relative_y,
            Direction::DownLeft => relative_y,
            Direction::DownRight => relative_x,
            _ => 0.0,
        };

        layer_elevation + layer_relative_elevation
    }

    pub fn ignore_tile_above_f32(&self, tile_position: Vec2, layer_index: i32) -> bool {
        self.ignore_tile_above(tile_position.as_ivec2(), layer_index)
    }

    pub fn ignore_tile_above(&self, tile_position: IVec2, layer_index: i32) -> bool {
        if layer_index < 0 {
            return false;
        }

        let layer = match self.tile_layers.get(layer_index as usize) {
            Some(layer) => layer,
            _ => return false,
        };

        let tile = layer.tile_at(tile_position);
        let tile_meta = match self.tile_meta_for_tile(tile.gid) {
            Some(tile_meta) => tile_meta,
            _ => return false,
        };

        tile_meta.tile_class == TileClass::Stairs
    }

    //     bool HasShadow(sf::Vector2i tilePos, int layer);
    //     bool TileConceals(sf::Vector2i tilePos, int layer);
    //     bool IsConcealed(sf::Vector2i tilePos, int layer);

    pub fn tile_object_at(
        &self,
        point: Vec2,
        layer_index: i32,
        solid_only: bool,
    ) -> Option<Entity> {
        type Query<'a> = hecs::Without<(&'a Tile, &'a ObjectData, &'a Vec3), &'a Excluded>;
        let mut query = self.object_entities.query::<Query>();

        for (entity, (tile, data, world_pos)) in query.iter() {
            if solid_only && !data.object_type.is_solid() {
                continue;
            }

            if world_pos.z != layer_index as f32 {
                continue;
            }

            let tile_meta = match self.tile_meta_for_tile(tile.gid) {
                Some(tileset) => tileset,
                _ => continue,
            };
            let tileset = &tile_meta.tileset;

            let active_frame = match tile_meta.animator.current_frame() {
                Some(frame) => frame,
                // should almost never occur, will update next frame
                _ => continue,
            };

            let tile_size = self.tile_size;
            let sprite_size = active_frame.size();

            let position = data.position;
            let rotation = data.rotation;
            let size = data.size;

            let world_position = point - position;
            let mut screen_position = self.world_to_screen(world_position);

            // rotation
            if rotation != 0.0 {
                let magnitude = screen_position.x.hypot(screen_position.y);
                let ortho_radians = screen_position.y.atan2(screen_position.x);
                let rotation_radians = -rotation / 180.0 * std::f32::consts::PI;
                screen_position.x = (ortho_radians + rotation_radians).cos() * magnitude;
                screen_position.y = (ortho_radians + rotation_radians).sin() * magnitude;
            }

            // scale
            let scale = size / sprite_size;
            screen_position /= scale;

            // calculate offset separately as we need to transform it separately
            let mut screen_offset = -tileset.alignment_offset;

            // not sure why this is needed
            // something to do with being centered at tile_size.x / 2
            screen_offset.x -= (tile_size.x / 2) as f32;

            // flip
            if tile.flipped_horizontal {
                screen_offset.x *= -1.0;

                // starting from the right side of the image during a horizontal flip
                screen_offset.x -=
                    sprite_size.x + tileset.alignment_offset.x + tileset.drawing_offset.x;

                // account for being incorrect?
                screen_offset.x -= ((tile_size.x / 2
                    - (sprite_size.x + tileset.drawing_offset.x) as i32)
                    / 2) as f32;

                // what??
                screen_offset.x -=
                    (tile_size.x / 2) as f32 * (tileset.alignment_offset.x / sprite_size.x);
            }

            if tile.flipped_vertical {
                screen_offset.y *= -1.0;

                // might be similar to the flip above, but not sure
                screen_offset.y -=
                    (tile_size.y * 2 - (sprite_size.y + tileset.drawing_offset.y) as i32) as f32;
            }

            // skipping tile rotation, editor doesn't allow this

            // adjust for the sprite being aligned at the bottom of the tile
            screen_offset.y -= sprite_size.y - tile_size.y as f32;

            screen_position.x += screen_offset.x;
            screen_position.y += screen_offset.y;

            let world_position = self.screen_to_world(screen_position);

            if tile.intersects(self, world_position) {
                return Some(entity);
            }
        }

        None
    }

    pub fn update(&mut self, world_time: FrameTime) {
        // update tile animations
        for meta in self.tile_metas.iter_mut().filter_map(|meta| meta.as_mut()) {
            meta.animator.sync_time(world_time);
        }

        // apply tile animations to tile objects
        for (_, (tile, sprite)) in self.object_entities.query_mut::<(&Tile, &mut Sprite)>() {
            let tile_meta = self
                .tile_metas
                .get(tile.gid as usize)
                .and_then(|o| o.as_ref());

            if let Some(tile_meta) = tile_meta {
                let original_origin = sprite.origin();
                tile_meta.animator.apply(sprite);
                sprite.set_origin(original_origin);
            }
        }

        if !self.tiles_modified {
            return;
        }

        self.calculate_shadows();
        self.tiles_modified = false;
    }

    fn calculate_shadows(&mut self) {
        // store a blank shadow map to get around mutability issues
        let mut shadow_map = ShadowMap::new(0, 0);
        std::mem::swap(&mut self.shadow_map, &mut shadow_map);

        shadow_map.calculate_shadows(self);

        // swap back
        std::mem::swap(&mut self.shadow_map, &mut shadow_map);
    }

    pub fn iterate_visible_tiles<F>(
        &self,
        game_io: &GameIO,
        camera: &Camera,
        layer_index: usize,
        mut callback: F,
    ) where
        F: FnMut(&mut Sprite, &TileMeta, Tile, (i32, i32)),
    {
        let layer = match self.tile_layer(layer_index) {
            Some(layer) => layer,
            _ => return,
        };

        let mut tile_sprite: Option<Sprite> = None;

        const TILE_PADDING: i32 = 3;
        let screen_size = camera.size();
        let tile_size = self.tile_size;

        let horizontal_tile_count = (screen_size.x / tile_size.x as f32).ceil() as i32;
        let vertical_tile_count = ((screen_size.y / tile_size.y as f32) * 2.0).ceil() as i32;

        let screen_top_left = camera.bounds().top_left();

        let tile_space_start = self.world_to_tile_space(self.screen_to_world(screen_top_left));
        let tile_space_start_i = tile_space_start.as_ivec2();

        let vertical_offset = layer_index as i32;

        for i in -TILE_PADDING..vertical_tile_count + TILE_PADDING {
            let vertical_start = (vertical_offset + i) / 2;
            let row_offset = (vertical_offset + i) % 2;

            for j in -TILE_PADDING..horizontal_tile_count + row_offset + TILE_PADDING {
                let col = tile_space_start_i.x + vertical_start + j;
                let row = tile_space_start_i.y + vertical_start - j + row_offset;

                let tile = layer.tile_at((col, row).into());

                // skip tiles with missing meta information
                let tile_meta = match self.tile_meta_for_tile(tile.gid) {
                    Some(tile_meta) => tile_meta,
                    None => continue,
                };

                // skip invisible tiles
                if tile_meta.tile_class == TileClass::Invisible {
                    continue;
                }

                // get or init the sprite we're reusing for drawing tiles
                let tile_sprite = match tile_sprite.as_mut() {
                    Some(tile_sprite) => {
                        tile_sprite.set_texture(tile_meta.tileset.texture.clone());
                        tile_sprite
                    }
                    None => {
                        tile_sprite = Some(Sprite::new(game_io, tile_meta.tileset.texture.clone()));
                        tile_sprite.as_mut().unwrap()
                    }
                };

                tile_meta.animator.apply(tile_sprite);
                let sprite_bounds = tile_sprite.size();

                let pos_i = IVec2::new((col * tile_size.x) / 2, row * tile_size.y);
                let screen_pos = self.world_to_screen(pos_i.as_vec2());
                let tile_offset = Vec2::new(
                    (-tile_size.x / 2 + sprite_bounds.x as i32 / 2) as f32,
                    (tile_size.y + tile_size.y / 2 - sprite_bounds.y as i32) as f32,
                );

                let mut position = screen_pos + tile_meta.drawing_offset + tile_offset;
                position.y -= tile_size.y as f32 * 0.5 * layer_index as f32;

                tile_sprite.set_position(position);

                tile_sprite.set_origin(Vec2::new(
                    (sprite_bounds.x as i32 / 2) as f32,
                    (tile_size.y / 2) as f32,
                ));

                tile_sprite.set_rotation(if tile.rotated {
                    std::f32::consts::FRAC_PI_2
                } else {
                    0.0
                });

                tile_sprite.set_scale(Vec2::new(
                    if tile.flipped_horizontal { -1.0 } else { 1.0 },
                    if tile.flipped_vertical { -1.0 } else { 1.0 },
                ));

                callback(tile_sprite, tile_meta, *tile, (col, row))
            }
        }
    }

    pub fn draw_tile_layer(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        camera: &Camera,
        layer_index: usize,
    ) {
        const SHADOW_COLOR: Color = Color::new(0.65, 0.65, 0.65, 1.0);

        self.iterate_visible_tiles(game_io, camera, layer_index, |sprite, _, _, (col, row)| {
            if self.shadow_map.has_shadow((col, row), layer_index) {
                sprite.set_color(SHADOW_COLOR);
            } else {
                sprite.set_color(Color::WHITE);
            }

            sprite_queue.draw_sprite(sprite);
        });
    }

    /// sorts and renders actors with tile objects
    /// actors must have a WorldPositon and Sprite
    pub fn draw_objects_with_entities(
        &self,
        sprite_queue: &mut SpriteColorQueue,
        entities: &hecs::World,
        layer_index: usize,
    ) {
        use std::cmp::Ordering;

        let mut queries = [entities, &self.object_entities]
            .map(|entities| entities.query::<hecs::Without<(&Sprite, &Vec3), &Excluded>>());

        let mut render_order = Vec::new();

        for query in &mut queries {
            for (_, (sprite, position)) in query.iter() {
                // add sprites on the same layer, bump up sprites to the next layer if the sprite is on stairs
                if position.z.ceil() as usize == layer_index {
                    let sort_value = self.world_to_screen(position.xy()).y;

                    render_order.push((sprite, sort_value));
                }
            }
        }

        render_order.sort_unstable_by(|(_, sort_value_a), (_, sort_value_b)| {
            sort_value_a
                .partial_cmp(sort_value_b)
                .unwrap_or(Ordering::Equal)
        });

        for (sprite, _) in render_order {
            sprite_queue.draw_sprite(sprite)
        }
    }
}
