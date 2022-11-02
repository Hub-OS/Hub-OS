use framework::prelude::*;
use std::sync::Arc;

#[repr(C)]
#[derive(Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
pub struct TriangleVertex {
    pub vertex: [f32; 2],
    pub color: Color,
}

impl Vertex for TriangleVertex {
    fn vertex_layout() -> VertexLayout {
        VertexLayout::new(&[VertexFormat::Float32x2, VertexFormat::Float32x4])
    }
}

#[repr(C)]
#[derive(Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
pub struct TriangleInstanceData {}

impl InstanceData for TriangleInstanceData {
    fn instance_layout() -> InstanceLayout {
        InstanceLayout::new(&[])
    }
}

pub struct Triangle {
    mesh: Arc<Mesh<TriangleVertex>>,
}

impl Triangle {
    pub fn new() -> Self {
        Self {
            mesh: Mesh::new(
                &[
                    TriangleVertex {
                        vertex: [0.0, 0.8],
                        color: Color::RED,
                    },
                    TriangleVertex {
                        vertex: [-0.8, -0.8],
                        color: Color::BLUE,
                    },
                    TriangleVertex {
                        vertex: [0.8, -0.8],
                        color: Color::GREEN,
                    },
                ],
                &[0, 1, 2],
            ),
        }
    }
}

impl Instance<TriangleInstanceData> for Triangle {
    fn instance_data(&self) -> TriangleInstanceData {
        TriangleInstanceData {}
    }

    fn instance_resources(&self) -> Vec<Arc<dyn AsBinding>> {
        Vec::new()
    }
}

impl Model<TriangleVertex, TriangleInstanceData> for Triangle {
    fn mesh(&self) -> &Arc<Mesh<TriangleVertex>> {
        &self.mesh
    }
}
