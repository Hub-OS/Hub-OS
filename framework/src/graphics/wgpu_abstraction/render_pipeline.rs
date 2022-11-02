use std::marker::PhantomData;
use std::sync::Arc;

pub struct RenderPipeline<Vertex: super::Vertex, InstanceData: super::InstanceData> {
    pub(super) render_pipeline: Arc<wgpu::RenderPipeline>,
    pub(super) uniform_bind_group_layout: wgpu::BindGroupLayout,
    pub(super) instance_bind_group_layout: wgpu::BindGroupLayout,
    vertex_phantom: PhantomData<Vertex>,
    instance_phantom: PhantomData<InstanceData>,
}

impl<Vertex: super::Vertex, InstanceData: super::InstanceData>
    RenderPipeline<Vertex, InstanceData>
{
    pub(super) fn new(
        render_pipeline: wgpu::RenderPipeline,
        uniform_bind_group_layout: wgpu::BindGroupLayout,
        instance_bind_group_layout: wgpu::BindGroupLayout,
    ) -> Self {
        Self {
            render_pipeline: Arc::new(render_pipeline),
            uniform_bind_group_layout,
            instance_bind_group_layout,
            vertex_phantom: PhantomData,
            instance_phantom: PhantomData,
        }
    }
}

impl<Vertex: super::Vertex, InstanceData: super::InstanceData>
    AsRef<RenderPipeline<Vertex, InstanceData>> for RenderPipeline<Vertex, InstanceData>
{
    fn as_ref(&self) -> &Self {
        self
    }
}
