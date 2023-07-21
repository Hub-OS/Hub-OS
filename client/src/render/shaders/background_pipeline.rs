use crate::render::*;
use framework::prelude::*;
use std::sync::Arc;

pub struct BackgroundPipeline {
    render_pipeline: RenderPipeline<Vec2, BackgroundInstanceData>,
    mesh: Arc<Mesh<Vec2>>,
}

impl BackgroundPipeline {
    pub fn new(game_io: &GameIO) -> Self {
        let device = game_io.graphics().device();

        let shader = device.create_shader_module(include_wgsl!("background_shader.wgsl"));

        let render_pipeline = RenderPipelineBuilder::new(game_io)
            .with_instance_bind_group(SpritePipeline::<()>::instance_bind_group_layout())
            .with_vertex_shader(&shader, "vs_main")
            .with_fragment_shader(&shader, "fs_main")
            .build::<Vec2, BackgroundInstanceData>()
            .unwrap();

        Self {
            render_pipeline,
            mesh: Self::create_mesh(game_io),
        }
    }

    fn create_mesh(game_io: &GameIO) -> Arc<Mesh<Vec2>> {
        Mesh::new(
            game_io,
            &[
                Vec2::new(0.0, 0.0),
                Vec2::new(0.0, 1.0),
                Vec2::new(1.0, 1.0),
                Vec2::new(1.0, 0.0),
            ],
            &[0, 1, 2, 2, 0, 3],
        )
    }

    pub fn mesh(&self) -> &Arc<Mesh<Vec2>> {
        &self.mesh
    }
}

impl AsRef<RenderPipeline<Vec2, BackgroundInstanceData>> for BackgroundPipeline {
    fn as_ref(&self) -> &RenderPipeline<Vec2, BackgroundInstanceData> {
        &self.render_pipeline
    }
}

/// RenderQueues only render when consumed by a RenderPass
pub struct BackgroundQueue<'a> {
    render_pipeline: &'a BackgroundPipeline,
    render_queue: RenderQueue<'a, Vec2, BackgroundInstanceData>,
}

impl<'a> BackgroundQueue<'a> {
    pub fn new(game_io: &'a GameIO, background_pipeline: &'a BackgroundPipeline) -> Self {
        Self {
            render_pipeline: background_pipeline,
            render_queue: RenderQueue::new(game_io, background_pipeline, vec![]),
        }
    }

    pub fn draw_background(&mut self, background: &Background) {
        let mesh = self.render_pipeline.mesh();
        self.render_queue.draw_instance(mesh, background);
    }
}

impl<'a> RenderQueueTrait for BackgroundQueue<'a> {
    fn into_operation_vec(self) -> Vec<RenderOperation> {
        self.render_queue.into_operation_vec()
    }
}
