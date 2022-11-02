use crate::bindable::SpriteColorMode;
use crate::ease::inverse_lerp;
use crate::packages::PackageNamespace;
use crate::render::ui::{draw_clock, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use crate::scenes::*;
use framework::prelude::*;
use num_derive::FromPrimitive;
use rand::Rng;
use std::ops::Range;
use strum::{EnumCount, EnumIter, IntoEnumIterator};

#[derive(Default)]
struct Particle {
    lifetime: FrameTime,
    max_lifetime: FrameTime,
    startup_delay: FrameTime,
    lifetime_range: Range<FrameTime>,
}

#[derive(Default)]
struct Stretching {
    scale_in: bool,
    scale_out: bool,
}

#[derive(Default)]
struct Motion {
    acceleration: Vec2,
    velocity: Vec2,
    friction: Vec2,
}

#[derive(Clone, Copy, EnumIter, EnumCount, FromPrimitive)]
enum MenuItem {
    Servers,
    Character,
    Folders,
    Library,
    BattleSelect,
    Config,
}

#[derive(Clone)]
struct BarrelItem {
    index: i32,
    offset: Vec2,
}

pub struct MainMenuScene {
    camera: Camera,
    background: Background,
    character_entity: hecs::Entity,
    character_shadow_entity: hecs::Entity,
    selected_item: i32,
    selection_scroll: f32,
    entities: hecs::World,
    ui_input_tracker: UiInputTracker,
    next_scene: NextScene<Globals>,
}

impl MainMenuScene {
    pub fn new(game_io: &mut GameIO<Globals>) -> MainMenuScene {
        let globals = game_io.globals();
        let assets = &globals.assets;
        let background_sampler = &globals.background_sampler;

        // setup camera
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // background
        let background_texture = assets.texture(game_io, ResourcePaths::MAIN_MENU_BG);
        let background_sprite = Sprite::new(background_texture, background_sampler.clone());
        let background = Background::new(Animator::new(), background_sprite);

        // menu assets
        let menu_sprite = assets.new_sprite(game_io, ResourcePaths::MAIN_MENU_PARTS);
        let mut menu_animator =
            Animator::load_new(assets, ResourcePaths::MAIN_MENU_PARTS_ANIMATION);

        // entities
        let mut entities = hecs::World::new();

        // folder particles
        let folder_sprite = assets.new_sprite(game_io, ResourcePaths::MAIN_MENU_FOLDER);
        let folder_anim = Animator::load_new(assets, ResourcePaths::MAIN_MENU_FOLDER_ANIMATION);

        let mut rng = rand::thread_rng();

        for _ in 0..12 {
            let mut animation = folder_anim.clone();
            animation.set_state(&rng.gen_range(0..3).to_string());

            let particle = Particle {
                lifetime_range: 180..900,
                ..Default::default()
            };

            entities.spawn((
                folder_sprite.clone(),
                animation,
                particle,
                Stretching::default(),
                Motion::default(),
            ));
        }

        // window particles
        let window_particle_count = 3;
        let window_sprite = assets.new_sprite(game_io, ResourcePaths::MAIN_MENU_WINDOWS);

        let mut window_anim =
            Animator::load_new(assets, ResourcePaths::MAIN_MENU_WINDOWS_ANIMATION);
        window_anim.set_state("default");

        let max_window_lifetime = (window_particle_count * 3 * 60 + 1) as i64;
        for _ in 0..window_particle_count {
            let animation = window_anim.clone();

            let particle = Particle {
                lifetime_range: 480..960,
                startup_delay: rng.gen_range(0..max_window_lifetime),
                ..Default::default()
            };

            entities.spawn((window_sprite.clone(), animation, particle));
        }

        // load the player character
        let mut character_sprite = load_character_sprite(game_io);
        let character_entity = entities.spawn((character_sprite.clone(),));

        character_sprite.set_origin(Vec2::new(20.0, 0.0));
        character_sprite.set_color(Color::from((0, 0, 20, 20)));
        let character_shadow_entity = entities.spawn((character_sprite,));

        // top bar
        let mut top_bar_sprite = menu_sprite.clone();
        menu_animator.set_state("TOP_BAR");
        menu_animator.apply(&mut top_bar_sprite);

        entities.spawn((top_bar_sprite,));

        // menu items
        for (i, menu_item) in MenuItem::iter().enumerate() {
            let mut label_animator = menu_animator.clone();
            let mut icon_animator = menu_animator.clone();

            match menu_item {
                MenuItem::Servers => {
                    label_animator.set_state("SERVERS_LABEL");
                    icon_animator.set_state("SERVERS");
                }
                MenuItem::Character => {
                    label_animator.set_state("CHARACTER_LABEL");
                    icon_animator.set_state("CHARACTER");
                }
                MenuItem::Folders => {
                    label_animator.set_state("FOLDERS_LABEL");
                    icon_animator.set_state("FOLDERS");
                }
                MenuItem::Library => {
                    label_animator.set_state("LIBRARY_LABEL");
                    icon_animator.set_state("LIBRARY");
                }
                MenuItem::BattleSelect => {
                    label_animator.set_state("BATTLE_SELECT_LABEL");
                    icon_animator.set_state("BATTLE_SELECT");
                }
                MenuItem::Config => {
                    label_animator.set_state("CONFIG_LABEL");
                    icon_animator.set_state("CONFIG");
                }
            }

            menu_animator.set_loop_mode(AnimatorLoopMode::Loop);

            let mut barrel_item = BarrelItem {
                index: i as i32,
                offset: Vec2::ZERO,
            };

            entities.spawn((menu_sprite.clone(), label_animator, barrel_item.clone()));

            barrel_item.offset.x = -15.0;

            entities.spawn((menu_sprite.clone(), icon_animator, barrel_item));
        }

        MainMenuScene {
            camera,
            background,
            character_entity,
            character_shadow_entity,
            selected_item: 0,
            selection_scroll: 0.0,
            entities,
            ui_input_tracker: UiInputTracker::new(),
            next_scene: NextScene::None,
        }
    }
}

fn load_character_sprite(game_io: &GameIO<Globals>) -> Sprite {
    let globals = game_io.globals();
    let assets = &globals.assets;

    let player_id = &globals.global_save.selected_character;
    let player_package = globals
        .player_packages
        .package(PackageNamespace::Local, player_id)
        .unwrap();

    let mut character_sprite = assets.new_sprite(game_io, &player_package.preview_texture_path);
    character_sprite.set_position(Vec2::new(0.0, RESOLUTION_F.y - character_sprite.size().y));

    character_sprite
}

impl Scene<Globals> for MainMenuScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO<Globals>) {
        let mut character_sprite = load_character_sprite(game_io);

        // update character
        let sprite = self
            .entities
            .query_one_mut::<&mut Sprite>(self.character_entity)
            .unwrap();
        *sprite = character_sprite.clone();

        // update character's shadow
        character_sprite.set_origin(Vec2::new(20.0, 0.0));

        let shadow_sprite = self
            .entities
            .query_one_mut::<&mut Sprite>(self.character_shadow_entity)
            .unwrap();
        *shadow_sprite = character_sprite;
        shadow_sprite.set_color(Color::from((0, 0, 20, 20)));
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        self.camera.update(game_io);

        self.background.update();

        // input
        if !game_io.is_in_transition() {
            self.ui_input_tracker.update(game_io);

            if self.ui_input_tracker.is_active(Input::Up) {
                self.selected_item -= 1;

                let globals = game_io.globals();
                globals.audio.play_sound(&globals.cursor_move_sfx);
            }

            if self.ui_input_tracker.is_active(Input::Down) {
                self.selected_item += 1;

                let globals = game_io.globals();
                globals.audio.play_sound(&globals.cursor_move_sfx);
            }

            if self.ui_input_tracker.is_active(Input::Confirm) {
                self.select_item(game_io);
            }
        }

        // systems
        system_animate(self.true_selection(), &mut self.entities);
        system_barrel(self);
        system_particle_motion(game_io, &mut self.entities);
        system_particle_stretching(&mut self.entities);
        system_age_particle(&mut self.entities);
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        // draw ui
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw entities
        for (_, sprite) in self.entities.query_mut::<&Sprite>() {
            sprite_queue.draw_sprite(sprite);
        }

        draw_clock(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

impl MainMenuScene {
    fn true_selection(&self) -> i32 {
        let item_count = MenuItem::COUNT as i32;

        (self.selected_item % item_count + item_count) % item_count
    }

    fn select_item(&mut self, game_io: &mut GameIO<Globals>) {
        use crate::transitions::*;
        use num_traits::FromPrimitive;

        let true_selection = self.true_selection();

        let scene: Option<Box<dyn Scene<Globals>>> =
            match MenuItem::from_i32(true_selection).unwrap() {
                MenuItem::Servers => Some(ServerListScene::new(game_io)),
                MenuItem::Folders => Some(FolderListScene::new(game_io)),
                MenuItem::Library => Some(LibraryScene::new(game_io)),
                MenuItem::BattleSelect => Some(BattleSelectScene::new(game_io)),
                _ => None,
            };

        let globals = game_io.globals();

        if let Some(scene) = scene {
            globals.audio.play_sound(&globals.cursor_select_sfx);

            let transition = PushTransition::new(
                game_io,
                game_io.globals().default_sampler.clone(),
                Direction::Right,
                DEFAULT_PUSH_DURATION,
            );

            self.next_scene = NextScene::Push {
                scene,
                transition: Some(Box::new(transition)),
            };
        } else {
            globals.audio.play_sound(&globals.cursor_error_sfx);
            log::warn!("Not implemented");
        }
    }
}

fn system_animate(true_selection: i32, entities: &mut hecs::World) {
    for (_, (particle, sprite, animator)) in
        entities.query_mut::<(&Particle, &mut Sprite, &mut Animator)>()
    {
        animator.sync_time(particle.lifetime);
        animator.apply(sprite);
    }

    for (_, (sprite, animator, barrel_item)) in
        entities.query_mut::<(&mut Sprite, &mut Animator, &BarrelItem)>()
    {
        if barrel_item.index == true_selection {
            animator.set_frame(1);
        } else {
            animator.set_frame(0);
        }

        animator.apply(sprite);
    }
}

fn system_barrel(scene: &mut MainMenuScene) {
    use std::f32::consts::PI;

    const RADIUS: f32 = 50.0;
    const SCALE: Vec2 = Vec2::new(1.3, 1.0);
    const OFFSET: Vec2 = Vec2::new(RESOLUTION_F.x - RESOLUTION_F.x / 10.0, RESOLUTION_F.y * 0.5);
    const ANGLE_RANGE: f32 = PI;

    let selection_f32 = scene.selected_item as f32;

    scene.selection_scroll =
        scene.selection_scroll + (selection_f32 - scene.selection_scroll) * 0.2;

    let item_count_f = MenuItem::COUNT as f32;
    let relative_scroll = (scene.selection_scroll % item_count_f + item_count_f) % item_count_f;

    for (_, (sprite, barrel_item)) in scene.entities.query_mut::<(&mut Sprite, &BarrelItem)>() {
        let relative_index =
            calc_relative_index(barrel_item.index as f32, relative_scroll, item_count_f);
        let angle_scalar = (relative_index as f32) / item_count_f;

        // calculate scale + color effect
        let abs_dist = angle_scalar.abs();

        let scale = 1.0 - abs_dist * 1.5;
        sprite.set_scale(Vec2::new(scale, scale));

        let mut color = sprite.color();
        color.a = 1.0 - abs_dist;
        sprite.set_color(color);

        // calculate position
        let angle = -ANGLE_RANGE * angle_scalar + PI;

        let position =
            (Vec2::from_angle(angle) * RADIUS + barrel_item.offset * scale) * SCALE + OFFSET;
        sprite.set_position(position);
    }
}

fn calc_relative_index<T>(a: T, b: T, len: T) -> T
where
    T: num_traits::NumOps + num_traits::Signed + std::cmp::PartialOrd + Copy,
{
    let two = T::one() + T::one();
    let mut relative_index = a - b;

    if relative_index.abs() > len / two {
        relative_index = -relative_index.signum() * len + relative_index
    }

    relative_index
}

fn system_particle_motion(game_io: &GameIO<Globals>, entities: &mut hecs::World) {
    //   sf::RenderWindow& window = getController().getWindow();
    //   sf::Vector2i mousei = sf::Mouse::getPosition(window);
    //   sf::Vector2f mousef = window.mapPixelToCoords(mousei);
    let mouse = game_io.input().mouse();

    let mut rng = rand::thread_rng();

    // Find particles that are dead or unborn
    for (_, (sprite, particle, motion)) in
        entities.query_mut::<(&mut Sprite, &Particle, &mut Motion)>()
    {
        if particle.lifetime == 0 {
            motion.acceleration = Vec2::new(rng.gen_range(-4.0..4.0), rng.gen_range(-7.5..4.0));
            motion.velocity = Vec2::new(rng.gen_range(-0.5..0.5), rng.gen_range(-0.5..0.5));
            motion.friction = Vec2::new(rng.gen_range(0.5..0.99), rng.gen_range(0.5..0.99));
        }

        // fly away from the mouse
        let mut position = sprite.position();

        let mut v = position - mouse;
        let length = v.length();
        let max_dist = 50.0;

        if length > 0.0 {
            v /= length;
        }

        let dropoff = (1.0 - (length / max_dist)).clamp(0.0, 1.0);

        let flee_speed = 5.0;
        motion.velocity += v * flee_speed * dropoff;
        motion.velocity += motion.acceleration / 60.0;

        position += motion.velocity / 60.0 * motion.friction;

        sprite.set_position(position);
    }
}

fn system_particle_stretching(entities: &mut hecs::World) {
    let mut rng = rand::thread_rng();

    for (_, (particle, stretch, sprite)) in
        entities.query_mut::<(&Particle, &mut Stretching, &mut Sprite)>()
    {
        if particle.lifetime == 0 {
            stretch.scale_in = rng.gen();
            stretch.scale_out = rng.gen();
        }

        let mut scale_out = particle.lifetime * 2 > particle.max_lifetime;
        let mut scale_in = !scale_out;

        scale_out = scale_out && stretch.scale_out;
        scale_in = scale_in && stretch.scale_in;

        let progress = inverse_lerp!(0.0, particle.max_lifetime as f32, particle.lifetime as f32);
        let beta = crate::ease::quadratic(progress);

        if scale_out || scale_in {
            sprite.set_scale(Vec2::new(beta, beta));
        } else {
            sprite.set_scale(Vec2::new(1.0, 1.0));
        }

        let mut color = sprite.color();
        color.a = beta * 0.392;
        sprite.set_color(color);
    }
}

fn system_age_particle(entities: &mut hecs::World) {
    let mut rng = rand::thread_rng();

    for (_, (particle, sprite)) in entities.query_mut::<(&mut Particle, &mut Sprite)>() {
        if particle.lifetime == 0 {
            sprite.set_position(Vec2::new(
                rng.gen_range(-5.0..5.0 + RESOLUTION_F.x),
                rng.gen_range(80.0..RESOLUTION_F.y),
            ));
        }

        if particle.startup_delay > 0 {
            particle.startup_delay -= 1;
            continue;
        }

        particle.lifetime += 1;

        if particle.lifetime > particle.max_lifetime {
            particle.lifetime = 0;
            particle.max_lifetime = rng.gen_range(particle.lifetime_range.clone());
        }
    }
}
