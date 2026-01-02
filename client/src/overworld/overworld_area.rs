use crate::overworld::components::*;
use crate::overworld::*;
use crate::render::ui::{FontName, Text, TextStyle};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use indexmap::IndexMap;
use packets::structures::ItemDefinition;
use std::sync::Arc;

pub struct OverworldArea {
    pub world_camera: Camera,
    pub ui_camera: Camera,
    pub player_data: OverworldPlayerData,
    pub item_registry: IndexMap<String, ItemDefinition>,
    pub entities: hecs::World,
    pub map: Map,
    pub last_map_update: FrameTime,
    pub emote_sprite: Sprite,
    pub emote_animator: Animator,
    pub event_sender: flume::Sender<OverworldEvent>,
    pub event_receiver: flume::Receiver<OverworldEvent>,
    pub world_time: FrameTime,
    pub visible: bool,
    input_locks: usize,
    background: Background,
    foreground: Background,
    camera_controller: CameraController,
}

impl OverworldArea {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut entities = hecs::World::new();

        // build the player
        let player_package = globals.global_save.player_package(game_io).unwrap();

        let texture = assets.texture(game_io, &player_package.overworld_paths.texture);
        let animator = Animator::load_new(assets, &player_package.overworld_paths.animation);
        let player_entity =
            Self::spawn_player_actor_direct(&mut entities, game_io, texture, animator, Vec3::ZERO);

        // enable the movement animator to actually move the player
        let movement_animator = entities
            .query_one_mut::<&mut MovementAnimator>(player_entity)
            .unwrap();
        movement_animator.set_movement_enabled(true);

        // data
        let player_data = OverworldPlayerData::new(
            game_io,
            player_entity,
            player_package.package_info.id.clone(),
        );

        // emote sprites
        let emote_animator = Animator::load_new(assets, ResourcePaths::OVERWORLD_EMOTES_ANIMATION);
        let emote_sprite = assets.new_sprite(game_io, ResourcePaths::OVERWORLD_EMOTES);

        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            world_camera: Camera::new(game_io),
            ui_camera: Camera::new_ui(game_io),
            player_data,
            item_registry: Default::default(),
            entities,
            map: Map::new(0, 0, 0, 0),
            last_map_update: 0,
            emote_sprite,
            emote_animator,
            event_sender,
            event_receiver,
            world_time: 0,
            input_locks: 0,
            visible: false,
            background: Background::new_blank(game_io),
            foreground: Background::new_blank(game_io),
            camera_controller: CameraController::new(player_entity),
        }
    }

    fn spawn_player_actor_direct(
        entities: &mut hecs::World,
        game_io: &GameIO,
        texture: Arc<Texture>,
        mut animator: Animator,
        position: Vec3,
    ) -> hecs::Entity {
        animator.set_state("IDLE_D");

        entities.spawn((
            Sprite::new(game_io, texture),
            animator,
            PlayerMapMarker::new_player(),
            MovementAnimator::new(),
            position,
            ActorCollider {
                radius: 4.0,
                solid: true,
            },
            Direction::Down,
            WarpController::default(),
        ))
    }

    pub fn spawn_player_actor(
        &mut self,
        game_io: &GameIO,
        texture: Arc<Texture>,
        animator: Animator,
        position: Vec3,
    ) -> hecs::Entity {
        Self::spawn_player_actor_direct(&mut self.entities, game_io, texture, animator, position)
    }

    pub fn spawn_artifact(
        &mut self,
        game_io: &GameIO,
        texture: Arc<Texture>,
        animator: Animator,
        position: Vec3,
    ) -> hecs::Entity {
        self.entities
            .spawn((Sprite::new(game_io, texture), animator, position))
    }

    pub fn despawn_sprite_attachments(&mut self, entity: hecs::Entity) {
        let attachment_iter = self.entities.query_mut::<&ActorAttachment>().into_iter();
        let pending_deletion: Vec<_> = attachment_iter
            .filter(|(_, attachment)| attachment.actor_entity == entity)
            .map(|(entity, _)| entity)
            .collect();

        for entity in pending_deletion {
            let _ = self.entities.despawn(entity);
        }
    }

    pub fn set_map(&mut self, game_io: &GameIO, assets: &impl AssetManager, map: Map) {
        if self.map.background_properties() != map.background_properties() {
            self.background = map
                .background_properties()
                .generate_background(game_io, assets);
        }

        if self.map.foreground_properties() != map.foreground_properties() {
            self.foreground = map
                .foreground_properties()
                .generate_background(game_io, assets);
        }

        self.map = map;
        self.last_map_update = self.world_time;
    }

    pub fn is_input_locked(&self, game_io: &GameIO) -> bool {
        game_io.is_in_transition()
            || self.camera_controller.is_locked()
            || self.input_locks > 0
            || self.player_is_warping()
            || self.animating_position()
    }

    fn animating_position(&self) -> bool {
        let entities = &self.entities;
        let Ok(mut query) = entities.query_one::<&ActorPropertyAnimator>(self.player_data.entity)
        else {
            return false;
        };

        matches!(query.get(), Some(property_animator) if property_animator.is_animating_position())
    }

    fn player_is_warping(&self) -> bool {
        let entities = &self.entities;
        let player_entity = self.player_data.entity;
        let mut query = entities
            .query_one::<&WarpController>(player_entity)
            .unwrap();
        let warp_controller = query.get().unwrap();

        warp_controller.warped_out
    }

    pub fn add_input_lock(&mut self) {
        self.input_locks += 1;
    }

    pub fn remove_input_lock(&mut self) {
        if self.input_locks > 0 {
            self.input_locks -= 1;
        }
    }

    pub fn queue_camera_action(&mut self, action: CameraAction) {
        self.camera_controller.queue_action(action);
    }

    fn handle_player_changes(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);
        let global_save = &globals.global_save;
        let player_package = global_save.player_package(game_io).unwrap();

        if self.player_data.package_id == player_package.package_info.id {
            return;
        }

        self.player_data.package_id = player_package.package_info.id.clone();
        self.player_data.health = self.player_data.max_health();
    }

    pub fn enter(&mut self, game_io: &GameIO) {
        self.player_data.process_boosts(game_io);
        self.handle_player_changes(game_io);
    }

    pub fn update(&mut self, game_io: &GameIO) {
        self.world_time += 1;

        self.map.update(self.world_time);
        self.update_backgrounds();

        self.ui_camera.update();
        self.camera_controller.update(
            game_io,
            &self.map,
            &mut self.entities,
            &mut self.world_camera,
        );
    }

    fn update_backgrounds(&mut self) {
        let player_entity = self.player_data.entity;
        let world_position = self.entities.query_one_mut::<&Vec3>(player_entity).unwrap();

        let screen_position = self.map.world_3d_to_screen(*world_position);

        let background_parallax = self.map.background_properties().parallax;
        self.background
            .set_offset(screen_position * background_parallax);
        self.background.update();

        let foreground_parallax = self.map.foreground_properties().parallax;
        self.foreground.update();
        self.foreground
            .set_offset(screen_position * foreground_parallax);
    }

    pub fn draw_screen_attachments<C: hecs::Component>(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        let mut query = self
            .entities
            .query::<(Option<&Sprite>, Option<&Text>, &AttachmentLayer, &C)>();
        let mut render_order = Vec::new();

        for (_, (sprite, text, &AttachmentLayer(layer), _)) in query.into_iter() {
            render_order.push((sprite, text, layer));
        }

        render_order.sort_by_key(|(_, _, layer)| -*layer);

        for (sprite, text, _) in render_order {
            if let Some(sprite) = sprite {
                sprite_queue.draw_sprite(sprite);
            }

            if let Some(text) = text {
                text.draw(game_io, sprite_queue);
            }
        }
    }

    pub fn draw_player_names_at(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        render_point: Vec2,
    ) {
        // find the closest named entity under the mouse
        let mut query = self.entities.query::<(&Sprite, &Vec3, &NameLabel)>();
        let query_iter = query.into_iter().map(|(_, pair)| pair);

        let camera_scale = self.world_camera.scale();
        let world_point = (render_point * Vec2::new(0.5, -0.5) + 0.5) * RESOLUTION_F * camera_scale;

        let screen_point =
            world_point + self.world_camera.position() - self.world_camera.size() * 0.5;

        let Some((.., name_label)) = query_iter
            .filter(|(.., name_label)| !name_label.0.is_empty())
            .filter(|(sprite, ..)| sprite.bounds().contains(screen_point))
            .max_by_key(|(_, position, _)| self.map.world_3d_to_screen(**position).y as i32)
        else {
            return;
        };

        // render the entity's name
        let name = &name_label.0;

        let mut text_style = TextStyle::new(game_io, FontName::Micro);

        // update bounds
        let text_size = text_style.measure(name).size;
        text_style.bounds.set_position(world_point);
        text_style.bounds -= text_size * Vec2::new(0.5, 1.0);
        text_style.bounds.y -= 2.0;

        // draw bg
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut bg_sprite = assets.new_sprite(game_io, ResourcePaths::PIXEL);
        bg_sprite.set_position(text_style.bounds.position() - 1.0);
        bg_sprite.set_size(text_size + 2.0);
        bg_sprite.set_color(Color::BLACK.multiply_alpha(0.5));
        sprite_queue.draw_sprite(&bg_sprite);

        // draw text
        text_style.draw(game_io, sprite_queue, name);
    }

    pub fn draw(
        &self,
        game_io: &GameIO,
        render_pass: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        // draw background
        self.background.draw(game_io, render_pass);

        // draw world
        let worlds = &[&self.entities, self.map.object_entities()];
        let sprite_layers = self.map.generate_sprite_layers(worlds);

        for (i, mut sprite_layer) in sprite_layers.into_iter().enumerate() {
            let layer_is_visible = self.map.tile_layer(i).is_some_and(|layer| layer.visible());

            if layer_is_visible {
                self.map
                    .draw_tile_layer(game_io, sprite_queue, &self.world_camera, i);
            }

            sprite_layer.draw(game_io, &self.map, worlds, sprite_queue);
        }

        player_interaction_debug_render(game_io, self, sprite_queue);

        // draw foreground
        self.foreground.draw(game_io, render_pass);

        // draw fade
        if self.world_camera.lens_tint().a != 0.0 {
            let globals = Globals::from_resources(game_io);
            let assets = &globals.assets;

            let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::PIXEL);
            fade_sprite.set_color(self.world_camera.lens_tint());
            fade_sprite.set_size(RESOLUTION_F);
            fade_sprite.set_origin(Vec2::new(0.5, 0.5));
            fade_sprite.set_position(self.world_camera.position());

            sprite_queue.draw_sprite(&fade_sprite);
        }
    }
}
