use framework::prelude::*;
use framework::wgpu;

pub struct CapturePass {
    encoder: wgpu::CommandEncoder,
    render_target: RenderTarget,
}

impl CapturePass {
    pub fn new(game_io: &GameIO, label: Option<&str>, size: UVec2) -> Self {
        let graphics = game_io.graphics().clone();
        let device = graphics.device();

        // build render target and pass
        let render_target = RenderTarget::new(&graphics, size);

        Self {
            encoder: device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label }),
            render_target,
        }
    }

    pub fn create_render_pass(&mut self) -> RenderPass<'_> {
        RenderPass::new(&mut self.encoder, &self.render_target)
    }

    pub fn capture(
        mut self,
        game_io: &GameIO,
    ) -> impl Future<Output = Option<image::ImageBuffer<image::Rgba<u8>, Vec<u8>>>> + use<> {
        // convert texture to rgba
        const FORMAT: wgpu::TextureFormat = wgpu::TextureFormat::Rgba8UnormSrgb;

        let converted_target =
            RenderTarget::new_with_format(game_io, self.render_target.size(), FORMAT);

        let graphics = game_io.graphics().clone();
        let device = graphics.device();
        let blitter = wgpu::util::TextureBlitter::new(device, FORMAT);
        blitter.copy(
            device,
            &mut self.encoder,
            self.render_target.texture().view(),
            converted_target.texture().view(),
        );

        // submit
        let command_buffer = self.encoder.finish();

        let queue = graphics.queue();
        queue.submit([command_buffer]);

        // capture render
        let (sender, receiver) = flume::unbounded();

        queue.on_submitted_work_done(move || {
            let _ = sender.send(());
        });

        async move {
            // wait for render to complete
            let _ = receiver.recv_async().await;

            // request bytes
            let bytes = converted_target.texture().read_bytes(&graphics).await;

            // add metadata for later usage
            let size = converted_target.size();
            image::ImageBuffer::<image::Rgba<u8>, Vec<u8>>::from_vec(size.x, size.y, bytes)
        }
    }
}
