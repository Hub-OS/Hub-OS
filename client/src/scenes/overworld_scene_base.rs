use crate::bindable::SpriteColorMode;
use crate::overworld::components::*;
use crate::overworld::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::sync::Arc;

pub struct OverworldSceneBase {
    pub world_camera: Camera,
    pub ui_camera: Camera,
    pub player_data: OverworldPlayerData,
    pub entities: hecs::World,
    pub map: Map,
    pub menu_manager: MenuManager,
    pub event_sender: flume::Sender<OverworldEvent>,
    pub event_receiver: flume::Receiver<OverworldEvent>,
    pub next_scene: NextScene,
    world_time: FrameTime,
    input_locks: usize,
    background: Background,
    foreground: Background,
    camera_controller: CameraController,
    health_ui: PlayerHealthUI,
}

impl OverworldSceneBase {
    pub fn new(game_io: &mut GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut entities = hecs::World::new();

        // build the player
        let player_package = globals.global_save.player_package(game_io).unwrap();

        let texture = assets.texture(game_io, &player_package.overworld_texture_path);
        let animator = Animator::load_new(assets, &player_package.overworld_animation_path);
        let player_entity =
            Self::spawn_player_actor_direct(&mut entities, game_io, texture, animator, Vec3::ZERO);

        // enable the movement animator to actually move the player
        let movement_animator = entities
            .query_one_mut::<&mut MovementAnimator>(player_entity)
            .unwrap();
        movement_animator.set_movement_enabled(true);

        let player_data = OverworldPlayerData::new(player_entity);
        let mut menu_manager = MenuManager::new(game_io);
        menu_manager.update_player_data(&player_data);

        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            world_camera: Camera::new(game_io),
            ui_camera: Camera::new_ui(game_io),
            player_data,
            entities,
            map: Map::new(0, 0, 0, 0),
            menu_manager,
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
            world_time: 0,
            input_locks: 0,
            background: Background::new_blank(game_io),
            foreground: Background::new_blank(game_io),
            camera_controller: CameraController::new(player_entity),
            health_ui: PlayerHealthUI::new(game_io),
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
            PlayerMinimapMarker::new_player(),
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

    pub fn set_world(&mut self, game_io: &GameIO, assets: &impl AssetManager, map: Map) {
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
    }

    pub fn is_input_locked(&self, game_io: &GameIO) -> bool {
        game_io.is_in_transition()
            || self.menu_manager.is_open()
            || self.camera_controller.is_locked()
            || self.input_locks > 0
            || self.player_is_warping()
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
        self.input_locks -= 1;
    }

    pub fn queue_camera_action(&mut self, action: CameraAction) {
        self.camera_controller.queue_action(action);
    }
}

impl Scene for OverworldSceneBase {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.world_time += 1;

        self.next_scene = self.menu_manager.update(game_io);
        self.health_ui.set_health(self.player_data.health);
        self.health_ui.update();

        system_player_movement(game_io, self);
        system_animate(self);
        system_position(self);
        system_player_interaction(game_io, self);
        system_warp_effect(game_io, self);
        system_warp(game_io, self);
        system_movement_animation(self);
        system_movement(self);

        self.map.update(self.world_time);
        self.update_backgrounds();

        self.ui_camera.update(game_io);
        self.camera_controller.update(
            game_io,
            &self.map,
            &mut self.entities,
            &mut self.world_camera,
        );
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        // draw sprites in world space
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.world_camera, SpriteColorMode::Multiply);

        for i in 0..self.map.tile_layers().len() {
            self.map
                .draw_tile_layer(game_io, &mut sprite_queue, &self.world_camera, i);

            self.map
                .draw_objects_with_entities(&mut sprite_queue, &self.entities, i);
        }

        // draw foreground
        self.foreground.draw(game_io, render_pass);

        // draw ui
        sprite_queue.update_camera(&self.ui_camera);

        if !self.menu_manager.is_blocking_hud() {
            draw_clock(game_io, &mut sprite_queue);
            draw_map_name(game_io, &mut sprite_queue, &self.map);
            self.health_ui.draw(game_io, &mut sprite_queue);
        }

        self.menu_manager.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

impl OverworldSceneBase {
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
