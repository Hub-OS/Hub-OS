use framework::logging::*;
use framework::prelude::*;
use rand::prelude::*;

type Globals = ();

fn main() -> anyhow::Result<()> {
    default_logger::init!();

    let game = Game::new("Sprites", (800, 600), |_| ());

    game.run(|game_io| MainScene::new(game_io))
}

struct MainScene {
    sprites: Vec<Sprite>,
    camera: OrthoCamera,
    render_pipeline: SpritePipeline<SpriteInstanceData>,
    next_scene: NextScene<Globals>,
}

impl MainScene {
    fn new(game_io: &mut GameIO<Globals>) -> Box<MainScene> {
        let mut camera = OrthoCamera::new(game_io, Vec2::new(800.0, 600.0));
        camera.invert_y(true);

        let texture = Texture::load_from_memory(game_io, include_bytes!("sprite.png")).unwrap();
        let sampler = Sprite::new_sampler(game_io);

        let mut sprites = Vec::new();
        let mut rng = rand::thread_rng();

        for _ in 0..50000 {
            let mut sprite = Sprite::new(texture.clone(), sampler.clone());

            let camera_bounds = camera.bounds();

            sprite.set_position(Vec2::new(
                rng.gen_range(camera_bounds.horizontal_range()),
                rng.gen_range(camera_bounds.vertical_range()),
            ));

            sprite.set_rotation(rng.gen_range(0.0..std::f32::consts::PI * 2.0));

            sprite.set_origin(Vec2::new(0.5, 0.5));
            sprite.set_size(Vec2::new(60.0, 60.0));

            sprites.push(sprite);
        }

        Box::new(MainScene {
            camera,
            render_pipeline: SpritePipeline::new(game_io, true),
            sprites,
            next_scene: NextScene::None,
        })
    }
}

impl Scene<Globals> for MainScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        let a = std::f32::consts::PI / 180.0 * 3.0;

        for sprite in &mut self.sprites {
            let rotation = sprite.rotation();
            sprite.set_rotation(rotation + a);
        }

        let mut camera_pos = self.camera.position();

        let input = game_io.input();

        camera_pos.x += input.controller_axis(0, AnalogAxis::LeftStickX)
            + input.keys_as_axis(Key::Left, Key::Right);

        camera_pos.y -= input.controller_axis(0, AnalogAxis::LeftStickY)
            + input.keys_as_axis(Key::Down, Key::Up);

        if input.was_button_just_pressed(0, Button::A) {
            input
                .controller(0)
                .unwrap()
                .rumble(1.0, 1.0, Duration::from_secs_f32(5.0));
        }

        self.camera.set_position(camera_pos);
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        // self.camera.resize_to_window(window);
        self.camera.scale_with_window(game_io.window());

        let uniforms = [self.camera.as_binding()];
        let mut render_queue = SpriteQueue::new(game_io, &self.render_pipeline, uniforms);

        for sprite in &self.sprites {
            render_queue.draw_sprite(sprite);
        }

        render_pass.consume_queue(render_queue);
    }
}
