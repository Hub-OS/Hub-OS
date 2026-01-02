use crate::resources::Globals;
use framework::{prelude::*, wgpu};

pub struct PostProcessGhosting {
    ghosting_resource: StructResource<f32>,
    active_target: RenderTarget,
    saved_target: RenderTarget,
    post_pipeline: PostPipeline,
    model: TextureSourceModel,
}

impl PostProcessGhosting {
    pub fn new(game_io: &GameIO) -> Self {
        let device = game_io.graphics().device();
        let shader = device.create_shader_module(include_wgsl!("post_process_ghosting.wgsl"));

        let globals = Globals::from_resources(game_io);

        let opacity_resource = StructResource::new(game_io, globals.post_process_ghosting);
        let active_target = RenderTarget::new(game_io, UVec2::new(1, 1));
        let saved_target = RenderTarget::new(game_io, UVec2::new(1, 1));
        let texture = saved_target.texture().clone();

        Self {
            ghosting_resource: opacity_resource,
            active_target,
            saved_target,
            post_pipeline: PostPipeline::new(
                game_io,
                &shader,
                "fs_main",
                &[BindGroupLayoutEntry {
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    binding_type: StructResource::<()>::binding_type(),
                }],
            ),
            model: TextureSourceModel::new(game_io, texture),
        }
    }
}

impl PostProcess for PostProcessGhosting {
    fn render_pipeline(&self) -> &PostPipeline {
        &self.post_pipeline
    }

    fn uniform_resources(&self) -> Vec<BindingResource<'_>> {
        Vec::new()
    }

    fn update(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);

        if *self.ghosting_resource.value() != globals.post_process_ghosting {
            self.ghosting_resource = StructResource::new(game_io, globals.post_process_ghosting);
        }
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        mut render_pass: RenderPass,
        texture_source: &TextureSourceModel,
    ) {
        let copy_pipeline = game_io.resource::<CopyPipeline>().unwrap();

        // resize target if necessary
        self.active_target
            .resize(game_io, texture_source.texture().size());

        // render to active target
        let mut sub_pass = render_pass.create_subpass(&self.active_target);

        // copy previous target to the new one
        let mut queue = RenderQueue::new(game_io, copy_pipeline, []);
        self.model.set_texture(self.saved_target.texture().clone());
        queue.draw_model(&self.model);
        sub_pass.consume_queue(queue);

        // render the source texture using the custom pipeline
        let mut queue = RenderQueue::new(
            game_io,
            self.render_pipeline(),
            [self.ghosting_resource.as_binding()],
        );
        queue.draw_model(texture_source);
        sub_pass.consume_queue(queue);

        sub_pass.flush();

        // copy final render
        let mut queue = RenderQueue::new(game_io, copy_pipeline, []);

        self.model.set_texture(self.active_target.texture().clone());
        queue.draw_model(&self.model);
        render_pass.consume_queue(queue);
        render_pass.flush();

        std::mem::swap(&mut self.active_target, &mut self.saved_target);
    }
}
