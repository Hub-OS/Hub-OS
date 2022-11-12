use crate::bindable::SpriteColorMode;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use framework::wgpu;
use std::sync::Arc;

pub struct SpritePipelineCollection {
    add_pipeline: SpritePipeline<SpriteInstanceData>,
    multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    greyscale_add_pipeline: SpritePipeline<SpriteInstanceData>,
    greyscale_multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    palette_add_pipeline: SpritePipeline<SpriteInstanceData>,
    palette_multiply_pipeline: SpritePipeline<SpriteInstanceData>,
}

impl SpritePipelineCollection {
    pub fn new(game_io: &GameIO<Globals>) -> Self {
        let device = game_io.graphics().device();
        let shared_shader = device.create_shader_module(include_wgsl!("sprite_color_shader.wgsl"));
        let palette_shader =
            device.create_shader_module(include_wgsl!("sprite_palette_shader.wgsl"));

        Self {
            add_pipeline: create_pipeline(game_io, &shared_shader, "add_main"),
            multiply_pipeline: create_pipeline(game_io, &shared_shader, "multiply_main"),
            greyscale_add_pipeline: create_pipeline(game_io, &shared_shader, "greyscale_add_main"),
            greyscale_multiply_pipeline: create_pipeline(
                game_io,
                &shared_shader,
                "greyscale_multiply_main",
            ),
            palette_add_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                &palette_shader,
                "add_main",
            ),
            palette_multiply_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                &palette_shader,
                "multiply_main",
            ),
        }
    }

    fn pipeline_for_config(
        &self,
        shader_effect: SpriteShaderEffect,
        color_mode: SpriteColorMode,
    ) -> &SpritePipeline<SpriteInstanceData> {
        match shader_effect {
            SpriteShaderEffect::Default => match color_mode {
                SpriteColorMode::Add => &self.add_pipeline,
                SpriteColorMode::Multiply => &self.multiply_pipeline,
            },
            SpriteShaderEffect::Greyscale => match color_mode {
                SpriteColorMode::Add => &self.greyscale_add_pipeline,
                SpriteColorMode::Multiply => &self.greyscale_multiply_pipeline,
            },
            SpriteShaderEffect::Palette => match color_mode {
                SpriteColorMode::Add => &self.palette_add_pipeline,
                SpriteColorMode::Multiply => &self.palette_multiply_pipeline,
            },
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

fn create_palette_pipeline(
    game_io: &GameIO<Globals>,
    vertex_shader: &wgpu::ShaderModule,
    palette_shader: &wgpu::ShaderModule,
    fragment_main: &str,
) -> SpritePipeline<SpriteInstanceData> {
    let render_pipeline = RenderPipelineBuilder::new(game_io)
        .with_uniform_bind_group(vec![
            OrthoCamera::bind_group_layout_entry(0),
            Texture::bind_group_layout_entry(1),
        ])
        .with_instance_bind_group_layout(vec![
            Texture::bind_group_layout_entry(0),
            TextureSampler::bind_group_layout_entry(1),
        ])
        .with_vertex_shader(vertex_shader, "vs_main")
        .with_fragment_shader(palette_shader, fragment_main)
        .build::<SpriteVertex, SpriteInstanceData>()
        .unwrap();

    SpritePipeline::from_custom_pipeline(render_pipeline, true)
}

/// RenderQueues only render when consumed by a RenderPass
pub struct SpriteColorQueue<'a> {
    sprite_queue: SpriteQueue<'a, SpriteInstanceData>,
    queue_draw_count: usize,
    operation_vec: Vec<RenderOperation>,
    shader_effect: SpriteShaderEffect,
    previous_shader_effect: SpriteShaderEffect,
    color_mode: SpriteColorMode,
    previous_color_mode: SpriteColorMode,
    camera: &'a Camera,
    updated_camera: bool,
    palette: Option<Arc<Texture>>,
    previous_palette_ptr: Option<*const Texture>,
    game_io: &'a GameIO<Globals>,
}

impl<'a> SpriteColorQueue<'a> {
    pub fn new(
        game_io: &'a GameIO<Globals>,
        camera: &'a Camera,
        color_mode: SpriteColorMode,
    ) -> Self {
        let shader_effect = SpriteShaderEffect::Default;
        let uniforms = [camera.as_binding()];

        let pipeline_collection = &game_io.globals().sprite_pipeline_collection;
        let pipeline = pipeline_collection.pipeline_for_config(shader_effect, color_mode);

        Self {
            sprite_queue: SpriteQueue::new(game_io, pipeline, uniforms),
            queue_draw_count: 0,
            operation_vec: Vec::new(),
            shader_effect,
            previous_shader_effect: shader_effect,
            color_mode,
            previous_color_mode: color_mode,
            camera,
            updated_camera: false,
            palette: None,
            previous_palette_ptr: None,
            game_io,
        }
    }

    pub fn color_mode(&self) -> SpriteColorMode {
        self.color_mode
    }

    pub fn set_color_mode(&mut self, color_mode: SpriteColorMode) {
        self.color_mode = color_mode;
    }

    pub fn shader_effect(&self) -> SpriteShaderEffect {
        self.shader_effect
    }

    pub fn set_shader_effect(&mut self, shader_effect: SpriteShaderEffect) {
        self.shader_effect = shader_effect;
    }

    pub fn update_camera(&mut self, camera: &'a Camera) {
        self.camera = camera;
        self.updated_camera = true;
    }

    pub fn set_palette(&mut self, texture: Arc<Texture>) {
        self.palette = Some(texture);
    }

    pub fn set_scissor(&mut self, rect: Rect) {
        self.sprite_queue.set_scissor(rect);
    }

    pub fn draw_sprite(&mut self, sprite: &Sprite) {
        let updated_shader_effect = self.shader_effect != self.previous_shader_effect;
        let updated_color_mode = self.color_mode != self.previous_color_mode;
        let requires_shader_change = updated_shader_effect || updated_color_mode;

        let palette_ptr = self.palette.as_ref().map(|p| Arc::as_ptr(p));
        let updated_palette = palette_ptr != self.previous_palette_ptr;
        let updated_uniforms = requires_shader_change || self.updated_camera || updated_palette;

        if updated_uniforms {
            let uniforms = if self.shader_effect == SpriteShaderEffect::Palette {
                vec![
                    self.camera.as_binding(),
                    self.palette.as_ref().unwrap().as_binding(),
                ]
            } else {
                vec![self.camera.as_binding()]
            };

            if requires_shader_change {
                // need to swap render pipelines
                let pipeline_collection = &self.game_io.globals().sprite_pipeline_collection;
                let pipeline =
                    pipeline_collection.pipeline_for_config(self.shader_effect, self.color_mode);

                let new_queue = SpriteQueue::new(self.game_io, pipeline, uniforms);
                let old_queue = std::mem::replace(&mut self.sprite_queue, new_queue);

                if self.queue_draw_count > 0 {
                    // need to store the operations if anything was drawn
                    self.operation_vec.extend(old_queue.into_operation_vec());
                    self.queue_draw_count = 0;
                }

                self.previous_shader_effect = self.shader_effect;
                self.previous_color_mode = self.color_mode;
            } else {
                // reusing the current pipeline and just updating the uniforms
                self.sprite_queue.set_uniforms(uniforms);
            }

            self.updated_camera = false;
            self.previous_palette_ptr = palette_ptr;
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
