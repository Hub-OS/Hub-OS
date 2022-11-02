use crate::render::*;
use framework::prelude::*;
use std::sync::Arc;

pub struct BackgroundPipeline {
    render_pipeline: RenderPipeline<FlatShapeVertex, BackgroundInstanceData>,
    mesh: Arc<Mesh<FlatShapeVertex>>,
}

impl BackgroundPipeline {
    pub fn new<Globals>(game_io: &GameIO<Globals>) -> Self {
        let device = game_io.graphics().device();

        let shader = device.create_shader_module(include_wgsl!("background_shader.wgsl"));

        let render_pipeline = RenderPipelineBuilder::new(game_io)
            .with_instance_bind_group_layout(vec![
                Texture::bind_group_layout_entry(0),
                TextureSampler::bind_group_layout_entry(1),
            ])
            .with_vertex_shader(&shader, "vs_main")
            .with_fragment_shader(&shader, "fs_main")
            .build::<FlatShapeVertex, BackgroundInstanceData>()
            .unwrap();

        Self {
            render_pipeline,
            mesh: Self::create_mesh(),
        }
    }

    fn create_mesh() -> Arc<Mesh<FlatShapeVertex>> {
        Mesh::new(
            &[
                FlatShapeVertex { vertex: [0.0, 0.0] },
                FlatShapeVertex { vertex: [0.0, 1.0] },
                FlatShapeVertex { vertex: [1.0, 1.0] },
                FlatShapeVertex { vertex: [1.0, 0.0] },
            ],
            &[0, 1, 2, 2, 0, 3],
        )
    }

    pub fn mesh(&self) -> &Arc<Mesh<FlatShapeVertex>> {
        &self.mesh
    }
}

impl AsRef<RenderPipeline<FlatShapeVertex, BackgroundInstanceData>> for BackgroundPipeline {
    fn as_ref(&self) -> &RenderPipeline<FlatShapeVertex, BackgroundInstanceData> {
        &self.render_pipeline
    }
}

/// RenderQueues only render when consumed by a RenderPass
pub struct BackgroundQueue<'a> {
    render_pipeline: &'a BackgroundPipeline,
    render_queue: RenderQueue<'a, FlatShapeVertex, BackgroundInstanceData>,
}

impl<'a> BackgroundQueue<'a> {
    pub fn new<Globals>(
        game_io: &'a GameIO<Globals>,
        background_pipeline: &'a BackgroundPipeline,
    ) -> Self {
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
