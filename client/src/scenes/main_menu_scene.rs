use crate::bindable::SpriteColorMode;
use crate::ease::inverse_lerp;
use crate::packages::PackageNamespace;
use crate::render::ui::{NavigationMenu, SceneOption};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use rand::Rng;
use std::ops::Range;

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

pub struct MainMenuScene {
    camera: Camera,
    background: Background,
    character_entity: hecs::Entity,
    character_shadow_entity: hecs::Entity,
    entities: hecs::World,
    navigation_menu: NavigationMenu,
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

        MainMenuScene {
            camera,
            background,
            character_entity,
            character_shadow_entity,
            entities,
            navigation_menu: NavigationMenu::new(
                game_io,
                vec![
                    SceneOption::Servers,
                    SceneOption::Folders,
                    SceneOption::Library,
                    SceneOption::Character,
                    SceneOption::BattleSelect,
                    SceneOption::Config,
                ],
            ),
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

        self.next_scene = self.navigation_menu.update(game_io);

        // systems
        system_animate(&mut self.entities);
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

        self.navigation_menu.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn system_animate(entities: &mut hecs::World) {
    for (_, (particle, sprite, animator)) in
        entities.query_mut::<(&Particle, &mut Sprite, &mut Animator)>()
    {
        animator.sync_time(particle.lifetime);
        animator.apply(sprite);
    }
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
