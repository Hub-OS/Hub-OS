use framework::prelude::*;
use rand::prelude::*;

pub type Globals = ();

pub struct MainScene {
    sprites: Vec<Sprite>,
    camera: OrthoCamera,
    render_pipeline: SpritePipeline<SpriteInstanceData>,
    next_scene: NextScene<Globals>,
}

impl MainScene {
    pub fn new(game_io: &mut GameIO<Globals>) -> Box<MainScene> {
        let mut camera = OrthoCamera::new(game_io, Vec2::new(800.0, 600.0));
        camera.invert_y(true);

        let render_pipeline = SpritePipeline::new(game_io, true);

        let texture = Texture::load_from_memory(game_io, include_bytes!("sprite.png")).unwrap();
        let sampler = Sprite::new_sampler(game_io);

        let mut sprites = Vec::new();
        let mut rng = rand::thread_rng();
        let camera_bounds = camera.bounds();
        let camera_horizontal_range = camera_bounds.horizontal_range();
        let camera_vertical_range = camera_bounds.vertical_range();

        for _ in 0..5000 {
            let mut sprite = Sprite::new(texture.clone(), sampler.clone());

            sprite.set_position(Vec2::new(
                rng.gen_range(camera_horizontal_range.clone()),
                rng.gen_range(camera_vertical_range.clone()),
            ));

            sprite.set_rotation(rng.gen_range(0.0..std::f32::consts::PI * 2.0));

            sprite.set_origin(sprite.size() * 0.5);

            sprites.push(sprite);
        }

        Box::new(MainScene {
            camera,
            render_pipeline,
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
