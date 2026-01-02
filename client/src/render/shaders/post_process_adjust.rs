use super::PostProcessAdjustConfig;
use crate::resources::Globals;
use framework::{prelude::*, wgpu};

pub struct PostProcessAdjust {
    config_resource: StructResource<PostProcessAdjustConfig>,
    pipeline: PostPipeline,
}

impl PostProcessAdjust {
    pub fn new(game_io: &GameIO) -> Self {
        let device = game_io.graphics().device();
        let shader = device.create_shader_module(include_wgsl!("post_process_adjust.wgsl"));

        let globals = Globals::from_resources(game_io);

        Self {
            config_resource: StructResource::new(game_io, globals.post_process_adjust_config),
            pipeline: PostPipeline::new(
                game_io,
                &shader,
                "fs_main",
                &[BindGroupLayoutEntry {
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    binding_type: StructResource::<()>::binding_type(),
                }],
            ),
        }
    }
}

impl PostProcess for PostProcessAdjust {
    fn render_pipeline(&self) -> &PostPipeline {
        &self.pipeline
    }

    fn uniform_resources(&self) -> Vec<BindingResource<'_>> {
        vec![self.config_resource.as_binding()]
    }

    fn update(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);

        if *self.config_resource.value() != globals.post_process_adjust_config {
            self.config_resource = StructResource::new(game_io, globals.post_process_adjust_config);
        }
    }
}
