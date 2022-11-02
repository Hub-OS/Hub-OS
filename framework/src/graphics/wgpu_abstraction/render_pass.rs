use super::*;
use crate::math::{Rect, UVec2};
use std::cell::RefCell;

/// "RenderPasses only render when flushed"
pub struct RenderPass<'a> {
    encoder: &'a RefCell<wgpu::CommandEncoder>,
    label: Option<&'a str>,
    color_attachments: Vec<Option<wgpu::RenderPassColorAttachment<'a>>>,
    depth_attachment: Option<wgpu::RenderPassDepthStencilAttachment<'a>>,
    queues: Vec<Vec<RenderOperation>>,
    texture_size: UVec2,
    clear_color: Color,
}

impl<'a> RenderPass<'a> {
    pub(crate) fn new<'b: 'a>(
        encoder: &'a RefCell<wgpu::CommandEncoder>,
        render_target: &'b RenderTarget,
    ) -> Self {
        Self {
            encoder,
            label: None,
            color_attachments: vec![Some(render_target.color_attachment())],
            depth_attachment: render_target.depth_attachment(),
            queues: Vec::new(),
            texture_size: render_target.size(),
            clear_color: render_target.clear_color(),
        }
    }

    pub fn target_size(&self) -> UVec2 {
        self.texture_size
    }

    pub fn clear_color(&self) -> Color {
        self.clear_color
    }

    pub fn create_subpass<'b: 'a>(&'a self, render_target: &'b RenderTarget) -> Self {
        Self {
            encoder: self.encoder,
            label: Some("render_target_pass"),
            color_attachments: vec![Some(render_target.color_attachment())],
            depth_attachment: render_target.depth_attachment(),
            queues: Vec::new(),
            texture_size: render_target.size(),
            clear_color: render_target.clear_color(),
        }
    }

    pub fn consume_queue<RenderQueue: RenderQueueTrait>(&mut self, queue: RenderQueue) {
        let operations = queue.into_operation_vec();

        if !operations.is_empty() {
            self.queues.push(operations);
        }
    }

    pub fn flush(self) {
        let descriptor = wgpu::RenderPassDescriptor {
            label: self.label,
            color_attachments: &self.color_attachments,
            depth_stencil_attachment: self.depth_attachment,
        };

        let mut encoder = self.encoder.borrow_mut();
        let mut render_pass = encoder.begin_render_pass(&descriptor);

        for queue in &self.queues {
            // println!("RenderOperations: {}", queue.len());

            let mut scissor_set = false;

            for operation in queue {
                match operation {
                    RenderOperation::SetPipeline(render_pipeline) => {
                        render_pass.set_pipeline(render_pipeline.as_ref());
                        // println!("set pipeline");
                    }
                    RenderOperation::SetScissor(rect) => {
                        let rect = rect.scissor(Rect::UNIT) * self.texture_size.as_vec2();

                        // rounding to avoid precision issues, applying a minimum size to avoid wgpu complaints
                        render_pass.set_scissor_rect(
                            (rect.x.round() as u32).min(self.texture_size.x - 1),
                            (rect.y.round() as u32).min(self.texture_size.y - 1),
                            (rect.width.round() as u32).max(1),
                            (rect.height.round() as u32).max(1),
                        );

                        scissor_set = true;
                        // println!("set scissor");
                    }
                    RenderOperation::SetUniforms(bind_group) => {
                        render_pass.set_bind_group(0, bind_group, &[]);
                        // println!("set uniforms");
                    }
                    RenderOperation::SetMesh(rc) => {
                        let (vertex_buffer, index_buffer) = rc.as_ref();
                        render_pass.set_vertex_buffer(0, vertex_buffer.slice(..));
                        render_pass
                            .set_index_buffer(index_buffer.slice(..), wgpu::IndexFormat::Uint32);
                        // println!("change mesh");
                    }
                    RenderOperation::SetInstanceResources(bind_group) => {
                        render_pass.set_bind_group(1, bind_group, &[]);
                        // println!("set instance resources");
                    }
                    RenderOperation::Draw {
                        instance_buffer,
                        index_count,
                        instance_count,
                    } => {
                        render_pass.set_vertex_buffer(1, instance_buffer.slice(..));
                        render_pass.draw_indexed(0..*index_count, 0, 0..*instance_count);
                        // println!("draw");
                    }
                }
            }

            if scissor_set {
                render_pass.set_scissor_rect(0, 0, self.texture_size.x, self.texture_size.y);
            }
        }
    }
}
