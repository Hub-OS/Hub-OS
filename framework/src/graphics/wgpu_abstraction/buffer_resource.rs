use crate::prelude::*;
use std::sync::Arc;
use wgpu::util::DeviceExt;

pub struct BufferResource {
    queue: Arc<wgpu::Queue>,
    buffer: wgpu::Buffer,
}

impl BufferResource {
    pub fn new<Globals>(game_io: &GameIO<Globals>, data: &[u8]) -> Self {
        let graphics = game_io.graphics();
        let device = graphics.device();

        let buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: None,
            contents: data,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
        });

        Self {
            queue: graphics.queue().clone(),
            buffer,
        }
    }

    pub fn write(&self, offset: u64, data: &[u8]) {
        self.queue.write_buffer(&self.buffer, offset, data);
    }
}

impl AsBinding for BufferResource {
    fn as_binding(&self) -> BindingResource<'_> {
        self.buffer.as_entire_binding()
    }
}
