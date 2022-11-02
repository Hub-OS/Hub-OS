pub trait Vertex: bytemuck::Pod {
    fn vertex_layout() -> VertexLayout;
}

pub struct VertexLayout {
    size: wgpu::BufferAddress,
    attributes: Vec<wgpu::VertexAttribute>,
}

impl VertexLayout {
    pub fn new(attribute_formats: &[wgpu::VertexFormat]) -> VertexLayout {
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

        VertexLayout { size, attributes }
    }

    pub(super) fn attribute_len(&self) -> u32 {
        self.attributes.len() as u32
    }

    pub(super) fn build<Vertex>(&self) -> Result<wgpu::VertexBufferLayout<'_>, String> {
        let array_stride = std::mem::size_of::<Vertex>() as wgpu::BufferAddress;

        if array_stride < self.size {
            return Err(String::from(
                "VertexLayout definition is larger than Vertex",
            ));
        }

        Ok(wgpu::VertexBufferLayout {
            array_stride,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &self.attributes,
        })
    }
}
