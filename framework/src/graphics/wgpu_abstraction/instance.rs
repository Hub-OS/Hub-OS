use crate::prelude::*;
use std::sync::Arc;

pub trait Instance<InstanceData: self::InstanceData> {
    fn instance_data(&self) -> InstanceData;
    fn instance_resources(&self) -> Vec<Arc<dyn AsBinding>>;
}

pub trait InstanceData: bytemuck::Pod {
    fn instance_layout() -> InstanceLayout;
}

pub struct InstanceLayout {
    size: wgpu::BufferAddress,
    attributes: Vec<wgpu::VertexAttribute>,
}

impl InstanceLayout {
    pub fn new(attribute_formats: &[wgpu::VertexFormat]) -> InstanceLayout {
        let mut size: wgpu::BufferAddress = 0;
        let mut shader_location = 0;

        let attributes: Vec<wgpu::VertexAttribute> = attribute_formats
            .iter()
            .map(|format| {
                let attribute = wgpu::VertexAttribute {
                    offset: size,
                    shader_location,
                    format: *format,
                };

                size += format.size();
                shader_location += 1;

                attribute
            })
            .collect();

        InstanceLayout { size, attributes }
    }

    pub(super) fn offset_attribute_locations(&mut self, offset: u32) {
        for attribute in &mut self.attributes {
            attribute.shader_location += offset;
        }
    }

    pub(super) fn build<Vertex>(&self) -> Result<wgpu::VertexBufferLayout<'_>, String> {
        let array_stride = std::mem::size_of::<Vertex>() as wgpu::BufferAddress;

        if array_stride < self.size {
            return Err(String::from(
                "InstanceLayout definition is larger than Vertex",
            ));
        }

        Ok(wgpu::VertexBufferLayout {
            array_stride,
            step_mode: wgpu::VertexStepMode::Instance,
            attributes: &self.attributes,
        })
    }
}
