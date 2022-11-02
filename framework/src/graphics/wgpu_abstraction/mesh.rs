use std::cell::RefCell;
use std::sync::Arc;

pub struct Mesh<Vertex: super::Vertex> {
    vertices: Vec<Vertex>,
    indices: Vec<u32>,
    buffers: RefCell<Option<Arc<(wgpu::Buffer, wgpu::Buffer)>>>,
}

impl<Vertex: super::Vertex> Mesh<Vertex> {
    pub fn new(vertices: &[Vertex], indices: &[u32]) -> Arc<Self> {
        Arc::new(Self {
            vertices: vertices.to_vec(),
            indices: indices.to_vec(),
            buffers: RefCell::new(None),
        })
    }

    pub fn vertices(&self) -> &[Vertex] {
        &self.vertices
    }

    pub fn indices(&self) -> &[u32] {
        &self.indices
    }

    pub(super) fn buffers(&self, device: &wgpu::Device) -> Arc<(wgpu::Buffer, wgpu::Buffer)> {
        use wgpu::util::DeviceExt;

        let mut buffers_mut = self.buffers.borrow_mut();

        if let Some(buffers) = buffers_mut.as_ref() {
            return buffers.clone();
        }

        let buffers = Arc::new((
            device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some("vertex_buffer"),
                contents: bytemuck::cast_slice(&self.vertices[..]),
                usage: wgpu::BufferUsages::VERTEX,
            }),
            device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some("index_buffer"),
                contents: bytemuck::cast_slice(&self.indices[..]),
                usage: wgpu::BufferUsages::INDEX,
            }),
        ));

        *buffers_mut = Some(buffers.clone());

        buffers
    }
}
