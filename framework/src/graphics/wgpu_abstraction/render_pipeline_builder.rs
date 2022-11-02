use crate::prelude::*;

pub struct RenderPipelineBuilder<'a, Globals: 'static> {
    game_io: &'a GameIO<Globals>,
    uniform_bind_group_layout_entries: Vec<wgpu::BindGroupLayoutEntry>,
    instance_bind_group_layout_entries: Vec<wgpu::BindGroupLayoutEntry>,
    vertex_shader: Option<(&'a wgpu::ShaderModule, String)>,
    fragment_shader: Option<(&'a wgpu::ShaderModule, String)>,
    target_format: wgpu::TextureFormat,
    blend: Option<wgpu::BlendState>,
    primitive: wgpu::PrimitiveState,
    depth_stencil: Option<wgpu::DepthStencilState>,
    multisample: wgpu::MultisampleState,
}

impl<'a, Globals> RenderPipelineBuilder<'a, Globals> {
    pub fn new(game_io: &'a GameIO<Globals>) -> Self {
        Self {
            game_io,
            uniform_bind_group_layout_entries: Vec::new(),
            instance_bind_group_layout_entries: Vec::new(),
            vertex_shader: None,
            fragment_shader: None,
            target_format: game_io.graphics().surface_config().format,
            blend: Some(wgpu::BlendState::ALPHA_BLENDING),
            primitive: wgpu::PrimitiveState::default(),
            depth_stencil: None,
            multisample: wgpu::MultisampleState::default(),
        }
    }

    pub fn with_uniform_bind_group(mut self, entries: Vec<wgpu::BindGroupLayoutEntry>) -> Self {
        self.uniform_bind_group_layout_entries = entries;
        self
    }

    pub fn with_instance_bind_group_layout(
        mut self,
        layout: Vec<wgpu::BindGroupLayoutEntry>,
    ) -> Self {
        self.instance_bind_group_layout_entries = layout;
        self
    }

    pub fn with_vertex_shader(mut self, shader: &'a wgpu::ShaderModule, entry: &str) -> Self {
        self.vertex_shader = Some((shader, entry.to_string()));
        self
    }

    pub fn with_fragment_shader(mut self, shader: &'a wgpu::ShaderModule, entry: &str) -> Self {
        self.fragment_shader = Some((shader, entry.to_string()));
        self
    }

    pub fn with_target_format(mut self, format: wgpu::TextureFormat) -> Self {
        self.target_format = format;
        self
    }

    pub fn with_blend(mut self, blend: wgpu::BlendState) -> Self {
        self.blend = Some(blend);
        self
    }

    pub fn with_primitive(mut self, primitive: wgpu::PrimitiveState) -> Self {
        self.primitive = primitive;
        self
    }

    pub fn with_depth_stencil(mut self, depth_stencil: wgpu::DepthStencilState) -> Self {
        self.depth_stencil = Some(depth_stencil);
        self
    }

    pub fn with_multisample(mut self, multisample: wgpu::MultisampleState) -> Self {
        self.multisample = multisample;
        self
    }

    pub fn build<Vertex, InstanceData>(
        self,
    ) -> Result<super::RenderPipeline<Vertex, InstanceData>, String>
    where
        Vertex: super::Vertex,
        InstanceData: super::InstanceData,
    {
        let device = self.game_io.graphics().device();

        let uniform_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: None,
                entries: &self.uniform_bind_group_layout_entries,
            });

        let instance_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: None,
                entries: &self.instance_bind_group_layout_entries,
            });

        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: None,
            bind_group_layouts: &[&uniform_bind_group_layout, &instance_bind_group_layout],
            push_constant_ranges: &[],
        });

        let (vertex_shader, vertex_entry) = self
            .vertex_shader
            .ok_or_else(|| String::from("Missing vertex shader!"))?;

        let (fragment_shader, fragment_entry) = self
            .fragment_shader
            .ok_or_else(|| String::from("Missing fragment shader!"))?;

        let vertex_layout = Vertex::vertex_layout();
        let mut instance_layout = InstanceData::instance_layout();
        instance_layout.offset_attribute_locations(vertex_layout.attribute_len());

        let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: None,
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                buffers: &[
                    vertex_layout.build::<Vertex>()?,
                    instance_layout.build::<InstanceData>()?,
                ],
                module: vertex_shader,
                entry_point: vertex_entry.as_str(),
            },
            fragment: Some(wgpu::FragmentState {
                targets: &[Some(wgpu::ColorTargetState {
                    format: self.target_format,
                    blend: self.blend,
                    write_mask: wgpu::ColorWrites::ALL,
                })],
                module: fragment_shader,
                entry_point: fragment_entry.as_str(),
            }),
            primitive: self.primitive,
            depth_stencil: self.depth_stencil,
            multisample: self.multisample,
            multiview: None,
        });

        Ok(super::RenderPipeline::new(
            render_pipeline,
            uniform_bind_group_layout,
            instance_bind_group_layout,
        ))
    }
}
