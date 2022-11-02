use crate::prelude::*;
use std::sync::Arc;

pub struct SpritePipeline<SpriteData: InstanceData> {
    render_pipeline: RenderPipeline<SpriteVertex, SpriteData>,
    mesh: Arc<Mesh<SpriteVertex>>,
}

impl<SpriteData: InstanceData> SpritePipeline<SpriteData> {
    pub fn new<Globals>(game_io: &GameIO<Globals>, invert_y: bool) -> Self {
        let device = game_io.graphics().device();

        let shader = device.create_shader_module(include_wgsl!("sprite_shader.wgsl"));

        let render_pipeline = RenderPipelineBuilder::new(game_io)
            .with_uniform_bind_group(vec![OrthoCamera::bind_group_layout_entry(0)])
            .with_instance_bind_group_layout(vec![
                Texture::bind_group_layout_entry(0),
                TextureSampler::bind_group_layout_entry(1),
            ])
            .with_vertex_shader(&shader, "vs_main")
            .with_fragment_shader(&shader, "fs_main")
            .build::<SpriteVertex, SpriteData>()
            .unwrap();

        Self {
            render_pipeline,
            mesh: Self::create_mesh(invert_y),
        }
    }

    pub fn from_custom_pipeline(
        render_pipeline: RenderPipeline<SpriteVertex, SpriteData>,
        invert_y: bool,
    ) -> Self {
        Self {
            render_pipeline,
            mesh: Self::create_mesh(invert_y),
        }
    }

    fn create_mesh(invert_y: bool) -> Arc<Mesh<SpriteVertex>> {
        let (y1, y2) = if invert_y { (0.0, 1.0) } else { (1.0, 0.0) };

        Mesh::new(
            &[
                SpriteVertex {
                    vertex: [0.0, 0.0],
                    uv: [0.0, y1],
                },
                SpriteVertex {
                    vertex: [0.0, 1.0],
                    uv: [0.0, y2],
                },
                SpriteVertex {
                    vertex: [1.0, 1.0],
                    uv: [1.0, y2],
                },
                SpriteVertex {
                    vertex: [1.0, 0.0],
                    uv: [1.0, y1],
                },
            ],
            &[0, 1, 2, 2, 0, 3],
        )
    }

    pub fn mesh(&self) -> &Arc<Mesh<SpriteVertex>> {
        &self.mesh
    }
}

impl<SpriteData: InstanceData> AsRef<RenderPipeline<SpriteVertex, SpriteData>>
    for SpritePipeline<SpriteData>
{
    fn as_ref(&self) -> &RenderPipeline<SpriteVertex, SpriteData> {
        &self.render_pipeline
    }
}
