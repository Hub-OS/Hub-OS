use crate::prelude::*;

pub struct FlatShapePipeline {
    render_pipeline: RenderPipeline<FlatShapeVertex, FlatShapeInstanceData>,
}

impl FlatShapePipeline {
    pub fn new<Globals>(game_io: &GameIO<Globals>) -> Self {
        let device = game_io.graphics().device();

        let shader = device.create_shader_module(include_wgsl!("flat_shader.wgsl"));

        let render_pipeline = RenderPipelineBuilder::new(game_io)
            .with_uniform_bind_group(vec![OrthoCamera::bind_group_layout_entry(0)])
            .with_vertex_shader(&shader, "vs_main")
            .with_fragment_shader(&shader, "fs_main")
            .build::<FlatShapeVertex, FlatShapeInstanceData>()
            .unwrap();

        Self { render_pipeline }
    }
}

impl AsRef<RenderPipeline<FlatShapeVertex, FlatShapeInstanceData>> for FlatShapePipeline {
    fn as_ref(&self) -> &RenderPipeline<FlatShapeVertex, FlatShapeInstanceData> {
        &self.render_pipeline
    }
}
