use crate::prelude::*;

pub struct ColorResource {
    color: Color,
    buffer_resource: BufferResource,
}

impl ColorResource {
    pub fn new<Globals>(game_io: &GameIO<Globals>, color: Color) -> Self {
        Self {
            color,
            buffer_resource: BufferResource::new(game_io, bytemuck::cast_slice(&[color])),
        }
    }

    pub fn color(&self) -> Color {
        self.color
    }

    pub fn bind_group_layout_entry(binding: u32) -> wgpu::BindGroupLayoutEntry {
        wgpu::BindGroupLayoutEntry {
            binding,
            visibility: wgpu::ShaderStages::FRAGMENT,
            ty: wgpu::BindingType::Buffer {
                ty: wgpu::BufferBindingType::Uniform,
                has_dynamic_offset: false,
                min_binding_size: None,
            },
            count: None,
        }
    }
}

impl AsBinding for ColorResource {
    fn as_binding(&self) -> BindingResource<'_> {
        self.buffer_resource.as_binding()
    }
}
