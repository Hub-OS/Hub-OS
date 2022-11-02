use crate::prelude::*;
use std::sync::Arc;

pub struct Texture {
    pub(crate) size: UVec2,
    pub(crate) view: wgpu::TextureView,
}

impl Texture {
    pub fn bind_group_layout_entry(binding: u32) -> wgpu::BindGroupLayoutEntry {
        wgpu::BindGroupLayoutEntry {
            binding,
            visibility: wgpu::ShaderStages::FRAGMENT,
            ty: wgpu::BindingType::Texture {
                multisampled: false,
                view_dimension: wgpu::TextureViewDimension::D2,
                sample_type: wgpu::TextureSampleType::Float { filterable: true },
            },
            count: None,
        }
    }

    pub fn load_from_memory<Globals>(
        game_io: &GameIO<Globals>,
        bytes: &[u8],
    ) -> anyhow::Result<Arc<Self>> {
        let image = image::load_from_memory(bytes)?;
        let rgba_image = image.to_rgba8();
        let size = rgba_image.dimensions();
        let (width, height) = size;

        let graphics = game_io.graphics();
        let device = graphics.device();
        let queue = graphics.queue();

        let extent = wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        };

        let texture = device.create_texture(&wgpu::TextureDescriptor {
            label: None,
            size: extent,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8UnormSrgb,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
        });

        let image_copy_texture = wgpu::ImageCopyTexture {
            texture: &texture,
            mip_level: 0,
            origin: wgpu::Origin3d::ZERO,
            aspect: wgpu::TextureAspect::All,
        };

        let image_data_layout = wgpu::ImageDataLayout {
            offset: 0,
            bytes_per_row: std::num::NonZeroU32::new(4 * width),
            rows_per_image: std::num::NonZeroU32::new(height),
        };

        queue.write_texture(image_copy_texture, &rgba_image, image_data_layout, extent);

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());

        Ok(Arc::new(Self {
            size: size.into(),
            view,
        }))
    }

    pub fn width(&self) -> u32 {
        self.size.x
    }

    pub fn height(&self) -> u32 {
        self.size.y
    }

    pub fn size(&self) -> UVec2 {
        self.size
    }
}

impl AsBinding for Texture {
    fn as_binding(&self) -> BindingResource {
        wgpu::BindingResource::TextureView(&self.view)
    }
}
