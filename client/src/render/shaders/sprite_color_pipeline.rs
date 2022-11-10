use crate::bindable::SpriteColorMode;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use framework::wgpu;

pub struct SpritePipelineCollection {
    add_pipeline: SpritePipeline<SpriteInstanceData>,
    multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    replace_pipeline: SpritePipeline<SpriteInstanceData>,
}

impl SpritePipelineCollection {
    pub fn new(game_io: &GameIO<Globals>) -> Self {
        let device = game_io.graphics().device();
        let shader = device.create_shader_module(include_wgsl!("sprite_color_shader.wgsl"));

        Self {
            add_pipeline: create_pipeline(game_io, &shader, "add_main"),
            multiply_pipeline: create_pipeline(game_io, &shader, "multiply_main"),
            replace_pipeline: create_pipeline(game_io, &shader, "replace_main"),
        }
    }

    fn pipeline_for_color_mode(
        &self,
        color_mode: SpriteColorMode,
    ) -> &SpritePipeline<SpriteInstanceData> {
        match color_mode {
            SpriteColorMode::Add => &self.add_pipeline,
            SpriteColorMode::Multiply => &self.multiply_pipeline,
            SpriteColorMode::Replace => &self.replace_pipeline,
        }
    }
}

fn create_pipeline(
    game_io: &GameIO<Globals>,
    shader: &wgpu::ShaderModule,
    fragment_main: &str,
) -> SpritePipeline<SpriteInstanceData> {
    let render_pipeline = RenderPipelineBuilder::new(game_io)
        .with_uniform_bind_group(vec![OrthoCamera::bind_group_layout_entry(0)])
        .with_instance_bind_group_layout(vec![
            Texture::bind_group_layout_entry(0),
            TextureSampler::bind_group_layout_entry(1),
        ])
        .with_vertex_shader(shader, "vs_main")
        .with_fragment_shader(shader, fragment_main)
        .build::<SpriteVertex, SpriteInstanceData>()
        .unwrap();

    SpritePipeline::from_custom_pipeline(render_pipeline, true)
}

/// RenderQueues only render when consumed by a RenderPass
pub struct SpriteColorQueue<'a> {
    sprite_queue: SpriteQueue<'a, SpriteInstanceData>,
    queue_draw_count: usize,
    operation_vec: Vec<RenderOperation>,
    color_mode: SpriteColorMode,
    previous_color_mode: SpriteColorMode,
    camera: &'a Camera,
    updated_camera: bool,
    game_io: &'a GameIO<Globals>,
}

impl<'a> SpriteColorQueue<'a> {
    pub fn new(
        game_io: &'a GameIO<Globals>,
        camera: &'a Camera,
        color_mode: SpriteColorMode,
    ) -> Self {
        let uniforms = [camera.as_binding()];

        let pipeline_collection = &game_io.globals().sprite_pipeline_collection;
        let pipeline = pipeline_collection.pipeline_for_color_mode(color_mode);

        Self {
            sprite_queue: SpriteQueue::new(game_io, pipeline, uniforms),
            queue_draw_count: 0,
            operation_vec: Vec::new(),
            color_mode,
            previous_color_mode: color_mode,
            camera,
            updated_camera: false,
            game_io,
        }
    }

    pub fn color_mode(&self) -> SpriteColorMode {
        self.color_mode
    }

    pub fn set_color_mode(&mut self, color_mode: SpriteColorMode) {
        self.color_mode = color_mode;
    }

    pub fn update_camera(&mut self, camera: &'a Camera) {
        self.camera = camera;
        self.updated_camera = true;
    }

    pub fn set_scissor(&mut self, rect: Rect) {
        self.sprite_queue.set_scissor(rect);
    }

    pub fn draw_sprite(&mut self, sprite: &Sprite) {
        let updated_color_mode = self.color_mode != self.previous_color_mode;
        let updated_uniforms = updated_color_mode || self.updated_camera;

        if updated_uniforms {
            let uniforms = [self.camera.as_binding()];

            if updated_color_mode {
                // need to swap render pipelines
                let pipeline_collection = &self.game_io.globals().sprite_pipeline_collection;
                let pipeline = pipeline_collection.pipeline_for_color_mode(self.color_mode);

                let new_queue = SpriteQueue::new(self.game_io, pipeline, uniforms);
                let old_queue = std::mem::replace(&mut self.sprite_queue, new_queue);

                if self.queue_draw_count > 0 {
                    // need to store the operations if anything was drawn
                    self.operation_vec.extend(old_queue.into_operation_vec());
                    self.queue_draw_count = 0;
                }

                self.previous_color_mode = self.color_mode;
            } else {
                // reusing the current pipeline and just updating the uniforms
                self.sprite_queue.set_uniforms(uniforms);
            }

            self.updated_camera = false;
        }

        self.sprite_queue.draw_sprite(sprite);
        self.queue_draw_count += 1;
    }
}

impl<'a> RenderQueueTrait for SpriteColorQueue<'a> {
    fn into_operation_vec(self) -> Vec<RenderOperation> {
        let mut operation_vec = self.operation_vec;

        if self.queue_draw_count > 0 {
            // need to store the operations if anything was drawn
            operation_vec.extend(self.sprite_queue.into_operation_vec());
        }

        operation_vec
    }
}
