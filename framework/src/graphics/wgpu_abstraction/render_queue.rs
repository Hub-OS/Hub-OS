use crate::prelude::*;
use std::sync::Arc;

#[derive(Debug)]
pub enum RenderOperation {
    SetPipeline(Arc<wgpu::RenderPipeline>),
    SetScissor(Rect),
    SetUniforms(wgpu::BindGroup),
    SetMesh(Arc<(wgpu::Buffer, wgpu::Buffer)>),
    SetInstanceResources(wgpu::BindGroup),
    Draw {
        instance_buffer: wgpu::Buffer,
        index_count: u32,
        instance_count: u32,
    },
}

/// RenderQueues only render when consumed by a RenderPass
pub struct RenderQueue<'a, Vertex: super::Vertex, InstanceData: super::InstanceData> {
    graphics: &'a GraphicsContext,
    uniform_bind_group_layout: &'a wgpu::BindGroupLayout,
    instance_bind_group_layout: &'a wgpu::BindGroupLayout,
    latest_mesh: Option<Arc<super::Mesh<Vertex>>>,
    latest_data: Vec<InstanceData>,
    latest_resources: Option<Vec<Arc<dyn AsBinding>>>,
    operations: Vec<RenderOperation>,
}

impl<'a, Vertex: super::Vertex, InstanceData: super::InstanceData>
    RenderQueue<'a, Vertex, InstanceData>
{
    pub fn new<'b, Globals, RenderPipeline, I>(
        game_io: &'a GameIO<Globals>,
        render_pipeline: &'a RenderPipeline,
        uniform_resources: I,
    ) -> Self
    where
        RenderPipeline: AsRef<super::RenderPipeline<Vertex, InstanceData>>,
        I: IntoIterator<Item = BindingResource<'b>>,
    {
        let render_pipeline = render_pipeline.as_ref();
        let wgpu_render_pipeline = render_pipeline.render_pipeline.clone();

        let mut render_queue = Self {
            graphics: game_io.graphics(),
            uniform_bind_group_layout: &render_pipeline.uniform_bind_group_layout,
            instance_bind_group_layout: &render_pipeline.instance_bind_group_layout,
            latest_mesh: None,
            latest_data: Vec::new(),
            latest_resources: None,
            operations: vec![RenderOperation::SetPipeline(wgpu_render_pipeline)],
        };

        render_queue.set_uniforms(uniform_resources);

        render_queue
    }

    pub fn set_uniforms<'b, I>(&mut self, uniform_resources: I)
    where
        I: IntoIterator<Item = BindingResource<'b>>,
    {
        let mut uniform_id = 0;
        let uniform_entries: Vec<wgpu::BindGroupEntry> = uniform_resources
            .into_iter()
            .map(|resource| {
                let entry = wgpu::BindGroupEntry {
                    binding: uniform_id,
                    resource,
                };

                uniform_id += 1;

                entry
            })
            .collect();

        self.create_uniform_bind_group(uniform_entries);
    }

    fn create_uniform_bind_group(&mut self, uniform_entries: Vec<wgpu::BindGroupEntry>) {
        let device = self.graphics.device();
        let uniform_bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("uniforms"),
            layout: self.uniform_bind_group_layout,
            entries: &uniform_entries,
        });

        // draw anything that relies on previous uniforms
        self.try_create_draw_call();

        self.operations
            .push(RenderOperation::SetUniforms(uniform_bind_group));
    }

    /// Rect will be clamped to fit within [0.0..1.0]. Applies to subsequent draw calls.
    pub fn set_scissor(&mut self, rect: Rect) {
        self.try_create_draw_call();
        self.operations.push(RenderOperation::SetScissor(rect));
    }

    pub fn draw_model<Model: super::Model<Vertex, InstanceData>>(&mut self, model: &Model) {
        let mesh = model.mesh();
        let data = model.instance_data();
        let resources = model.instance_resources();
        self.draw_mesh(mesh, data, resources)
    }

    pub fn draw_instance<Instance: super::Instance<InstanceData>>(
        &mut self,
        mesh: &Arc<super::Mesh<Vertex>>,
        instance: &Instance,
    ) {
        let data = instance.instance_data();
        let resources = instance.instance_resources();
        self.draw_mesh(mesh, data, resources)
    }

    pub fn draw_mesh(
        &mut self,
        mesh: &Arc<super::Mesh<Vertex>>,
        data: InstanceData,
        resources: Vec<Arc<dyn AsBinding>>,
    ) {
        let should_set_mesh = self.should_set_mesh(mesh);
        let should_set_resources = self.should_set_resources(&resources);

        if should_set_mesh || should_set_resources {
            // draw queued instances before setting new data
            self.try_create_draw_call();
        }

        if should_set_mesh {
            self.set_mesh(mesh);
        }

        if should_set_resources {
            self.set_resources(resources);
        }

        self.latest_data.push(data)
    }

    fn should_set_mesh(&self, mesh: &Arc<super::Mesh<Vertex>>) -> bool {
        if let Some(latest_mesh) = &self.latest_mesh {
            !Arc::ptr_eq(mesh, latest_mesh)
        } else {
            true
        }
    }

    fn should_set_resources(&self, resources: &Vec<Arc<dyn AsBinding>>) -> bool {
        let latest_resources = match self.latest_resources.as_ref() {
            Some(latest_resources) => latest_resources,
            // resources have not been set yet
            _ => return true,
        };

        if resources.is_empty() {
            // no need to swap if there's no resources
            return false;
        }

        // use if anything doesn't match
        !latest_resources
            .iter()
            .zip(resources)
            .all(|(a, b)| std::ptr::eq(a, b))
    }

    fn set_mesh(&mut self, mesh: &Arc<super::Mesh<Vertex>>) {
        let device = self.graphics.device();

        self.operations
            .push(RenderOperation::SetMesh(mesh.buffers(device)));

        self.latest_mesh = Some(mesh.clone());
    }

    fn set_resources(&mut self, bindings: Vec<Arc<dyn AsBinding>>) {
        let mut binding = 0;
        let resource_entries: Vec<wgpu::BindGroupEntry> = bindings
            .iter()
            .map(|resource| {
                let entry = wgpu::BindGroupEntry {
                    binding,
                    resource: resource.as_binding(),
                };

                binding += 1;

                entry
            })
            .collect();

        let device = self.graphics.device();
        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("instance_resources"),
            layout: self.instance_bind_group_layout,
            entries: &resource_entries,
        });

        self.operations
            .push(RenderOperation::SetInstanceResources(bind_group));

        self.latest_resources = Some(bindings);
    }

    fn try_create_draw_call(&mut self) {
        if self.latest_data.is_empty() {
            // nothing to draw
            return;
        }

        // create buffer for instance data
        use wgpu::util::DeviceExt;

        let device = self.graphics.device();
        let instance_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("instance_buffer"),
            contents: if std::mem::size_of::<InstanceData>() > 0 {
                bytemuck::cast_slice(&self.latest_data[..])
            } else {
                &[]
            },
            usage: wgpu::BufferUsages::VERTEX,
        });

        // actually make the draw call
        let index_count = self.latest_mesh.as_ref().unwrap().indices().len() as u32;
        let instance_count = self.latest_data.len() as u32;

        self.operations.push(RenderOperation::Draw {
            instance_buffer,
            index_count,
            instance_count,
        });

        // clear the data now that we moved it into a buffer and used it to calculate instance count
        self.latest_data.clear();
    }
}

impl<'a, Vertex: super::Vertex, InstanceData: super::InstanceData> RenderQueueTrait
    for RenderQueue<'a, Vertex, InstanceData>
{
    fn into_operation_vec(mut self) -> Vec<RenderOperation> {
        self.try_create_draw_call();

        self.operations
    }
}

pub trait RenderQueueTrait {
    fn into_operation_vec(self) -> Vec<RenderOperation>;
}
