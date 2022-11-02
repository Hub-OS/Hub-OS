mod triangle;

use framework::logging::*;
use framework::prelude::*;
use triangle::{Triangle, TriangleInstanceData, TriangleVertex};

type Globals = ();

fn main() -> anyhow::Result<()> {
    default_logger::init!();

    let game = Game::new("Triangle Example", (800, 600), |_| ());

    game.run(|game_io| MainScene::new(game_io))
}

struct MainScene {
    render_pipeline: RenderPipeline<TriangleVertex, TriangleInstanceData>,
    triangle: Triangle,
    next_scene: NextScene<Globals>,
}

impl MainScene {
    fn new(game_io: &mut GameIO<Globals>) -> Box<MainScene> {
        let graphics = game_io.graphics();

        let shader = graphics
            .load_shader_from_descriptor(include_wgsl!("triangle.wgsl"))
            .unwrap();

        let render_pipeline = RenderPipelineBuilder::new(&game_io)
            .with_vertex_shader(&shader, "vs_main")
            .with_fragment_shader(&shader, "fs_main")
            .build::<TriangleVertex, TriangleInstanceData>()
            .unwrap();

        Box::new(MainScene {
            render_pipeline,
            triangle: Triangle::new(),
            next_scene: NextScene::None,
        })
    }
}

impl Scene<Globals> for MainScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, _game_io: &mut GameIO<Globals>) {}

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        let mut render_queue = RenderQueue::new(game_io, &self.render_pipeline, []);
        render_queue.draw_model(&self.triangle);
        render_pass.consume_queue(render_queue);
    }
}
