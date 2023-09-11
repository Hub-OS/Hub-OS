// for online battles see netplay_init_scene.rs

use framework::prelude::*;

use crate::{battle::BattleProps, resources::Globals, transitions::BATTLE_HOLD_DURATION};

use super::BattleScene;

pub struct BattleInitScene {
    start_instant: Instant,
    camera: OrthoCamera,
    model: FlatModel,
    battle_scene: Option<BattleScene>,
    next_scene: NextScene,
}

impl BattleInitScene {
    pub fn new(game_io: &mut GameIO, props: BattleProps) -> Self {
        let mut camera = OrthoCamera::new(game_io, Vec2::ONE);
        camera.invert_y(false);

        let mut model = FlatModel::new_square_model(game_io);
        model.set_color(Color::WHITE);

        Self {
            start_instant: Instant::now(),
            camera,
            model,
            battle_scene: Some(BattleScene::new(game_io, props)),
            next_scene: NextScene::None,
        }
    }
}

impl Scene for BattleInitScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.start_instant = Instant::now();

        let globals = game_io.resource::<Globals>().unwrap();

        globals.audio.stop_music();
        globals.audio.play_sound(&globals.sfx.battle_transition);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        if game_io.is_in_transition() || self.start_instant.elapsed() < BATTLE_HOLD_DURATION {
            return;
        }

        if let Some(scene) = self.battle_scene.take() {
            let transition = crate::transitions::new_battle(game_io);
            self.next_scene = NextScene::new_swap(scene).with_transition(transition);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let render_pipeline = game_io.resource::<FlatPipeline>().unwrap();
        let mut flat_queue = RenderQueue::new(game_io, render_pipeline, [self.camera.as_binding()]);

        flat_queue.draw_model(&self.model);
        render_pass.consume_queue(flat_queue);
    }
}
