use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use framework::wgpu;

#[derive(Default, Clone, Copy, PartialEq, bytemuck::Zeroable, bytemuck::Pod)]
#[repr(C)]
pub struct MapTileUniforms {
    pub center: Vec2,
    pub tile_size: Vec2,
    pub trim: i32,
    pub padding: i32,
}

pub struct MapPipeline {
    tile_pipeline: SpritePipeline<SpriteInstanceData>,
    edge_pipeline: SpritePipeline<SpriteInstanceData>,
}

impl MapPipeline {
    pub fn new(game_io: &GameIO) -> Self {
        let device = game_io.graphics().device();
        let vertex_shader = device.create_shader_module(include_wgsl!("sprite_color_shader.wgsl"));
        let tile_fragment_shader =
            device.create_shader_module(include_wgsl!("map_tile_shader.wgsl"));
        let edge_fragment_shader =
            device.create_shader_module(include_wgsl!("map_edge_shader.wgsl"));

        let tile_pipeline = RenderPipelineBuilder::new(game_io)
            .with_uniform_bind_group(&[
                BindGroupLayoutEntry {
                    visibility: wgpu::ShaderStages::VERTEX,
                    binding_type: OrthoCamera::binding_type(),
                },
                BindGroupLayoutEntry {
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    binding_type: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                },
            ])
            .with_instance_bind_group(SpritePipeline::<()>::instance_bind_group_layout())
            .with_vertex_shader(&vertex_shader, "vs_main")
            .with_fragment_shader(&tile_fragment_shader, "fs_main")
            .build::<SpriteVertex, SpriteInstanceData>()
            .unwrap();

        let edge_pipeline = RenderPipelineBuilder::new(game_io)
            .with_instance_bind_group(SpritePipeline::<()>::instance_bind_group_layout())
            .with_uniform_bind_group(&[BindGroupLayoutEntry {
                visibility: wgpu::ShaderStages::VERTEX,
                binding_type: OrthoCamera::binding_type(),
            }])
            .with_vertex_shader(&vertex_shader, "vs_main")
            .with_fragment_shader(&edge_fragment_shader, "fs_main")
            .build::<SpriteVertex, SpriteInstanceData>()
            .unwrap();

        Self {
            tile_pipeline: SpritePipeline::from_custom_pipeline(tile_pipeline),
            edge_pipeline: SpritePipeline::from_custom_pipeline(edge_pipeline),
        }
    }
}

/// RenderQueues only render when consumed by a RenderPass
pub struct MapTileSpriteQueue<'a> {
    sprite_queue: SpriteQueue<'a, SpriteInstanceData>,
    tile_uniforms: MapTileUniforms,
    prev_tile_uniforms: MapTileUniforms,
    camera: &'a Camera,
    game_io: &'a GameIO,
}

impl<'a> MapTileSpriteQueue<'a> {
    pub fn new(game_io: &'a GameIO, camera: &'a Camera) -> Self {
        let tile_uniforms = MapTileUniforms::default();
        let buffer_resource = BufferResource::new(game_io, bytemuck::cast_slice(&[tile_uniforms]));
        let uniforms = [camera.as_binding(), buffer_resource.as_binding()];

        let globals = Globals::from_resources(game_io);
        let pipeline = &globals.map_pipeline.tile_pipeline;

        Self {
            sprite_queue: SpriteQueue::new(game_io, pipeline, uniforms).with_inverted_y(true),
            tile_uniforms,
            prev_tile_uniforms: tile_uniforms,
            camera,
            game_io,
        }
    }

    pub fn set_tile_uniforms(&mut self, map_uniforms: MapTileUniforms) {
        self.tile_uniforms = map_uniforms;
    }

    pub fn draw_sprite(&mut self, sprite: &Sprite) {
        let updated_uniforms = self.tile_uniforms != self.prev_tile_uniforms;

        if updated_uniforms {
            let buffer_resource =
                BufferResource::new(self.game_io, bytemuck::cast_slice(&[self.tile_uniforms]));

            self.sprite_queue
                .set_uniforms([self.camera.as_binding(), buffer_resource.as_binding()]);

            self.prev_tile_uniforms = self.tile_uniforms;
        }

        self.sprite_queue.draw_sprite(sprite);
    }
}

impl RenderQueueTrait for MapTileSpriteQueue<'_> {
    fn into_operation_vec(self) -> Vec<RenderOperation> {
        self.sprite_queue.into_operation_vec()
    }
}

/// RenderQueues only render when consumed by a RenderPass
pub struct MapSpriteQueue<'a> {
    sprite_queue: SpriteQueue<'a, SpriteInstanceData>,
    game_io: &'a GameIO,
}

impl<'a> MapSpriteQueue<'a> {
    pub fn new(game_io: &'a GameIO, camera: &Camera) -> Self {
        let globals = Globals::from_resources(game_io);
        let pipeline = &globals.map_pipeline.edge_pipeline;
        let uniforms = [camera.as_binding()];

        Self {
            sprite_queue: SpriteQueue::new(game_io, pipeline, uniforms).with_inverted_y(true),
            game_io,
        }
    }

    pub fn draw_sprite(&mut self, sprite: &Sprite) {
        self.sprite_queue.draw_sprite(sprite);
    }
}

impl RenderQueueTrait for MapSpriteQueue<'_> {
    fn into_operation_vec(self) -> Vec<RenderOperation> {
        self.sprite_queue.into_operation_vec()
    }
}
