use crate::bindable::SpriteColorMode;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use framework::wgpu;
use std::sync::Arc;

pub struct SpritePipelineCollection {
    add_pipeline: SpritePipeline<SpriteInstanceData>,
    multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    adopt_pipeline: SpritePipeline<SpriteInstanceData>,

    palette_add_pipeline: SpritePipeline<SpriteInstanceData>,
    palette_multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    palette_adopt_pipeline: SpritePipeline<SpriteInstanceData>,

    grayscale_add_pipeline: SpritePipeline<SpriteInstanceData>,
    grayscale_multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    grayscale_adopt_pipeline: SpritePipeline<SpriteInstanceData>,

    grayscale_palette_add_pipeline: SpritePipeline<SpriteInstanceData>,
    grayscale_palette_multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    grayscale_palette_adopt_pipeline: SpritePipeline<SpriteInstanceData>,

    pixelate_add_pipeline: SpritePipeline<SpriteInstanceData>,
    pixelate_multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    pixelate_adopt_pipeline: SpritePipeline<SpriteInstanceData>,

    pixelate_palette_add_pipeline: SpritePipeline<SpriteInstanceData>,
    pixelate_palette_multiply_pipeline: SpritePipeline<SpriteInstanceData>,
    pixelate_palette_adopt_pipeline: SpritePipeline<SpriteInstanceData>,
}

impl SpritePipelineCollection {
    pub fn new(game_io: &GameIO) -> Self {
        let device = game_io.graphics().device();
        let shared_shader = device.create_shader_module(include_wgsl!("sprite_color_shader.wgsl"));
        let palette_shader =
            device.create_shader_module(include_wgsl!("sprite_palette_shader.wgsl"));

        // reused for grayscale
        let plain_adopt = create_pipeline(game_io, &shared_shader, "vs_main", "adopt_main");
        let palette_adopt = create_palette_pipeline(
            game_io,
            &shared_shader,
            "vs_main",
            &palette_shader,
            "adopt_main",
        );

        Self {
            // plain
            add_pipeline: create_pipeline(game_io, &shared_shader, "vs_main", "add_main"),
            multiply_pipeline: create_pipeline(game_io, &shared_shader, "vs_main", "multiply_main"),
            adopt_pipeline: plain_adopt.clone(),

            // palette
            palette_add_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                "vs_main",
                &palette_shader,
                "add_main",
            ),
            palette_multiply_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                "vs_main",
                &palette_shader,
                "multiply_main",
            ),
            palette_adopt_pipeline: palette_adopt.clone(),

            // grayscale
            grayscale_add_pipeline: create_pipeline(
                game_io,
                &shared_shader,
                "vs_main",
                "grayscale_add_main",
            ),
            grayscale_multiply_pipeline: create_pipeline(
                game_io,
                &shared_shader,
                "vs_main",
                "grayscale_multiply_main",
            ),
            grayscale_adopt_pipeline: plain_adopt,

            // grayscale palette
            grayscale_palette_add_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                "vs_main",
                &palette_shader,
                "grayscale_add_main",
            ),
            grayscale_palette_multiply_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                "vs_main",
                &palette_shader,
                "grayscale_multiply_main",
            ),
            grayscale_palette_adopt_pipeline: palette_adopt.clone(),

            // pixelate
            pixelate_add_pipeline: create_pipeline(
                game_io,
                &shared_shader,
                "pixelate_vs_main",
                "pixelate_add_main",
            ),
            pixelate_multiply_pipeline: create_pipeline(
                game_io,
                &shared_shader,
                "pixelate_vs_main",
                "pixelate_multiply_main",
            ),
            pixelate_adopt_pipeline: create_pipeline(
                game_io,
                &shared_shader,
                "pixelate_vs_main",
                "pixelate_adopt_main",
            ),

            // pixelate palette
            pixelate_palette_add_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                "pixelate_vs_main",
                &palette_shader,
                "pixelate_add_main",
            ),
            pixelate_palette_multiply_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                "pixelate_vs_main",
                &palette_shader,
                "pixelate_multiply_main",
            ),
            pixelate_palette_adopt_pipeline: create_palette_pipeline(
                game_io,
                &shared_shader,
                "pixelate_vs_main",
                &palette_shader,
                "pixelate_adopt_main",
            ),
        }
    }

    fn pipeline_for_config(
        &self,
        shader_effect: SpriteShaderEffect,
        with_palette: bool,
        color_mode: SpriteColorMode,
    ) -> &SpritePipeline<SpriteInstanceData> {
        match shader_effect {
            SpriteShaderEffect::Default => {
                if with_palette {
                    match color_mode {
                        SpriteColorMode::Add => &self.palette_add_pipeline,
                        SpriteColorMode::Multiply => &self.palette_multiply_pipeline,
                        SpriteColorMode::Adopt => &self.palette_adopt_pipeline,
                    }
                } else {
                    match color_mode {
                        SpriteColorMode::Add => &self.add_pipeline,
                        SpriteColorMode::Multiply => &self.multiply_pipeline,
                        SpriteColorMode::Adopt => &self.adopt_pipeline,
                    }
                }
            }
            SpriteShaderEffect::Grayscale => {
                if with_palette {
                    match color_mode {
                        SpriteColorMode::Add => &self.grayscale_palette_add_pipeline,
                        SpriteColorMode::Multiply => &self.grayscale_palette_multiply_pipeline,
                        SpriteColorMode::Adopt => &self.grayscale_palette_adopt_pipeline,
                    }
                } else {
                    match color_mode {
                        SpriteColorMode::Add => &self.grayscale_add_pipeline,
                        SpriteColorMode::Multiply => &self.grayscale_multiply_pipeline,
                        SpriteColorMode::Adopt => &self.grayscale_adopt_pipeline,
                    }
                }
            }
            SpriteShaderEffect::Pixelate => {
                if with_palette {
                    match color_mode {
                        SpriteColorMode::Add => &self.pixelate_palette_add_pipeline,
                        SpriteColorMode::Multiply => &self.pixelate_palette_multiply_pipeline,
                        SpriteColorMode::Adopt => &self.pixelate_palette_adopt_pipeline,
                    }
                } else {
                    match color_mode {
                        SpriteColorMode::Add => &self.pixelate_add_pipeline,
                        SpriteColorMode::Multiply => &self.pixelate_multiply_pipeline,
                        SpriteColorMode::Adopt => &self.pixelate_adopt_pipeline,
                    }
                }
            }
        }
    }
}

fn create_pipeline(
    game_io: &GameIO,
    shader: &wgpu::ShaderModule,
    vertex_main: &str,
    fragment_main: &str,
) -> SpritePipeline<SpriteInstanceData> {
    let render_pipeline = RenderPipelineBuilder::new(game_io)
        .with_uniform_bind_group(&[BindGroupLayoutEntry {
            visibility: wgpu::ShaderStages::VERTEX,
            binding_type: OrthoCamera::binding_type(),
        }])
        .with_instance_bind_group(SpritePipeline::<()>::instance_bind_group_layout())
        .with_vertex_shader(shader, vertex_main)
        .with_fragment_shader(shader, fragment_main)
        .build::<SpriteVertex, SpriteInstanceData>()
        .unwrap();

    SpritePipeline::from_custom_pipeline(render_pipeline)
}

fn create_palette_pipeline(
    game_io: &GameIO,
    vertex_shader: &wgpu::ShaderModule,
    vertex_main: &str,
    palette_shader: &wgpu::ShaderModule,
    fragment_main: &str,
) -> SpritePipeline<SpriteInstanceData> {
    let render_pipeline = RenderPipelineBuilder::new(game_io)
        .with_uniform_bind_group(&[
            BindGroupLayoutEntry {
                visibility: wgpu::ShaderStages::VERTEX,
                binding_type: OrthoCamera::binding_type(),
            },
            BindGroupLayoutEntry {
                visibility: wgpu::ShaderStages::FRAGMENT,
                binding_type: wgpu::BindingType::Texture {
                    sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    view_dimension: wgpu::TextureViewDimension::D2,
                    multisampled: false,
                },
            },
        ])
        .with_instance_bind_group(SpritePipeline::<()>::instance_bind_group_layout())
        .with_vertex_shader(vertex_shader, vertex_main)
        .with_fragment_shader(palette_shader, fragment_main)
        .build::<SpriteVertex, SpriteInstanceData>()
        .unwrap();

    SpritePipeline::from_custom_pipeline(render_pipeline)
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
    game_io: &'a GameIO,
}

impl<'a> SpriteColorQueue<'a> {
    pub fn new(game_io: &'a GameIO, camera: &'a Camera, color_mode: SpriteColorMode) -> Self {
        let shader_effect = SpriteShaderEffect::Default;
        let uniforms = [camera.as_binding()];

        let globals = Globals::from_resources(game_io);
        let pipeline_collection = &globals.sprite_pipeline_collection;
        let pipeline = pipeline_collection.pipeline_for_config(shader_effect, false, color_mode);

        Self {
            sprite_queue: SpriteQueue::new(game_io, pipeline, uniforms).with_inverted_y(true),
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

    pub fn set_palette(&mut self, texture: Option<Arc<Texture>>) {
        self.palette = texture;
    }

    pub fn set_scissor(&mut self, rect: Rect) {
        self.sprite_queue.set_scissor(rect);
    }

    pub fn draw_sprite(&mut self, sprite: &Sprite) {
        let updated_shader_effect = self.shader_effect != self.previous_shader_effect;
        let updated_color_mode = self.color_mode != self.previous_color_mode;
        let palette_toggled = self.palette.is_some() != self.previous_palette_ptr.is_some();
        let requires_shader_change = updated_shader_effect || updated_color_mode || palette_toggled;

        let palette_ptr = self.palette.as_ref().map(Arc::as_ptr);
        let updated_palette = palette_ptr != self.previous_palette_ptr;
        let updated_uniforms = requires_shader_change || self.updated_camera || updated_palette;

        if updated_uniforms {
            let uniforms = if self.palette.is_some() {
                vec![
                    self.camera.as_binding(),
                    self.palette.as_ref().unwrap().as_binding(),
                ]
            } else {
                vec![self.camera.as_binding()]
            };

            if requires_shader_change {
                // need to swap render pipelines
                let globals = Globals::from_resources(self.game_io);
                let pipeline_collection = &globals.sprite_pipeline_collection;
                let pipeline = pipeline_collection.pipeline_for_config(
                    self.shader_effect,
                    self.palette.is_some(),
                    self.color_mode,
                );

                let new_queue =
                    SpriteQueue::new(self.game_io, pipeline, uniforms).with_inverted_y(true);
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

impl RenderQueueTrait for SpriteColorQueue<'_> {
    fn into_operation_vec(self) -> Vec<RenderOperation> {
        let mut operation_vec = self.operation_vec;

        if self.queue_draw_count > 0 {
            // need to store the operations if anything was drawn
            operation_vec.extend(self.sprite_queue.into_operation_vec());
        }

        operation_vec
    }
}
