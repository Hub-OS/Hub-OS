use crate::prelude::*;
use raw_window_handle::{HasRawDisplayHandle, HasRawWindowHandle};
use std::borrow::Cow;
use std::future::Future;
use std::path::Path;
use std::sync::Arc;

pub struct GraphicsContext {
    surface: wgpu::Surface,
    surface_config: wgpu::SurfaceConfiguration,
    adapter: Arc<wgpu::Adapter>,
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    clear_color: Color,
}

impl GraphicsContext {
    pub fn surface(&self) -> &wgpu::Surface {
        &self.surface
    }

    pub fn surface_config(&self) -> &wgpu::SurfaceConfiguration {
        &self.surface_config
    }

    pub fn surface_size(&self) -> UVec2 {
        UVec2::new(self.surface_config.width, self.surface_config.height)
    }

    pub fn adapter(&self) -> &Arc<wgpu::Adapter> {
        &self.adapter
    }

    pub fn device(&self) -> &Arc<wgpu::Device> {
        &self.device
    }

    pub fn queue(&self) -> &Arc<wgpu::Queue> {
        &self.queue
    }

    pub fn clear_color(&self) -> Color {
        self.clear_color
    }

    pub fn set_clear_color(&mut self, color: Color) {
        self.clear_color = color
    }

    pub fn load_wgsl<P: AsRef<Path> + ?Sized>(
        &self,
        path: &P,
    ) -> std::io::Result<
        SyncResultAsyncError<
            wgpu::ShaderModule,
            wgpu::Error,
            impl Future<Output = Option<wgpu::Error>>,
        >,
    > {
        let wgsl = std::fs::read_to_string(path)?;
        Ok(
            self.load_shader_from_descriptor(wgpu::ShaderModuleDescriptor {
                label: Some(path.as_ref().to_string_lossy().as_ref()),
                source: wgpu::ShaderSource::Wgsl(Cow::Borrowed(wgsl.as_str())),
            }),
        )
    }

    pub fn load_wgsl_from_str(
        &self,
        source: &str,
    ) -> SyncResultAsyncError<
        wgpu::ShaderModule,
        wgpu::Error,
        impl Future<Output = Option<wgpu::Error>>,
    > {
        self.load_shader_from_descriptor(wgpu::ShaderModuleDescriptor {
            label: None,
            source: wgpu::ShaderSource::Wgsl(Cow::Borrowed(source)),
        })
    }

    pub fn load_shader_from_descriptor(
        &self,
        descriptor: wgpu::ShaderModuleDescriptor,
    ) -> SyncResultAsyncError<
        wgpu::ShaderModule,
        wgpu::Error,
        impl Future<Output = Option<wgpu::Error>>,
    > {
        self.device.push_error_scope(wgpu::ErrorFilter::Validation);
        let shader = self.device.create_shader_module(descriptor);
        let error = self.device.pop_error_scope();

        SyncResultAsyncError::new(shader, error)
    }

    pub(crate) async fn new<Window: HasRawWindowHandle + HasRawDisplayHandle>(
        window: &Window,
        width: u32,
        height: u32,
    ) -> anyhow::Result<GraphicsContext> {
        let instance = {
            // https://github.com/gfx-rs/wgpu/issues/2384
            crate::cfg_android! {
                wgpu::Instance::new(wgpu::Backends::GL)
            }
            crate::cfg_desktop_and_web! {
                wgpu::Instance::new(wgpu::Backends::all())
            }
        };

        let surface = unsafe { instance.create_surface(window) };

        let adapter_opt = instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::HighPerformance,
                compatible_surface: Some(&surface),
                force_fallback_adapter: false,
            })
            .await;

        let adapter = adapter_opt.ok_or_else(|| anyhow::anyhow!("No adapter found"))?;

        let (device, queue) = adapter
            .request_device(
                &wgpu::DeviceDescriptor {
                    label: None,
                    limits: {
                        crate::cfg_web! {
                            wgpu::Limits::downlevel_webgl2_defaults()
                        }
                        crate::cfg_android! {
                            wgpu::Limits::downlevel_webgl2_defaults()
                        }
                        crate::cfg_desktop! {
                            wgpu::Limits::default()
                        }
                    },
                    features: wgpu::Features::empty(),
                },
                None,
            )
            .await?;

        let surface_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: surface.get_supported_formats(&adapter)[0],
            width,
            height,
            present_mode: wgpu::PresentMode::AutoVsync,
            alpha_mode: wgpu::CompositeAlphaMode::Auto,
        };
        surface.configure(&device, &surface_config);

        Ok(GraphicsContext {
            surface,
            surface_config,
            adapter: Arc::new(adapter),
            device: Arc::new(device),
            queue: Arc::new(queue),
            clear_color: Color::TRANSPARENT,
        })
    }

    pub(crate) fn resized(&mut self, size: UVec2) {
        self.surface_config.width = size.x;
        self.surface_config.height = size.y;

        self.surface.configure(&self.device, &self.surface_config);
    }
}
