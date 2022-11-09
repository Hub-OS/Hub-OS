use crate::prelude::*;
use std::sync::Arc;

#[derive(Clone)]
pub struct Sprite {
    texture: Arc<Texture>,
    sampler: Arc<TextureSampler>,
    origin: Vec2,
    position: Vec2,
    scale: Vec2,
    rotation: f32,
    frame: Rect,
    color: Color,
}

impl Sprite {
    pub fn new_sampler<Globals>(game_io: &GameIO<Globals>) -> Arc<TextureSampler> {
        TextureSampler::new(game_io, SamplingFilter::Nearest, EdgeSampling::Clamp)
    }

    pub fn new(texture: Arc<Texture>, sampler: Arc<TextureSampler>) -> Self {
        Self {
            texture,
            sampler,
            origin: Vec2::new(0.0, 0.0),
            position: Vec2::new(0.0, 0.0),
            scale: Vec2::new(1.0, 1.0),
            rotation: 0.0,
            frame: Rect::new(0.0, 0.0, 1.0, 1.0),
            color: Color::WHITE,
        }
    }

    pub fn texture(&self) -> &Arc<Texture> {
        &self.texture
    }

    pub fn set_texture(&mut self, texture: Arc<Texture>) {
        self.texture = texture;
    }

    pub fn sampler(&self) -> &Arc<TextureSampler> {
        &self.sampler
    }

    pub fn set_sampler(&mut self, sampler: Arc<TextureSampler>) {
        self.sampler = sampler;
    }

    pub fn origin(&self) -> Vec2 {
        self.origin
    }

    pub fn set_origin(&mut self, origin: Vec2) {
        self.origin = origin;
    }

    pub fn position(&self) -> Vec2 {
        self.position
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.position = position;
    }

    pub fn size(&self) -> Vec2 {
        Vec2::new(
            (self.scale.x * self.frame.width).abs() * self.texture.width() as f32,
            (self.scale.y * self.frame.height).abs() * self.texture.height() as f32,
        )
    }

    pub fn set_width(&mut self, width: f32) {
        self.scale.x = width / self.frame.width.abs() / self.texture.width() as f32;
    }

    pub fn set_height(&mut self, height: f32) {
        self.scale.y = height / self.frame.height.abs() / self.texture.height() as f32;
    }

    pub fn set_size(&mut self, size: Vec2) {
        self.scale.x = size.x / self.frame.width.abs() / self.texture.width() as f32;
        self.scale.y = size.y / self.frame.height.abs() / self.texture.height() as f32;
    }

    pub fn scale(&self) -> Vec2 {
        self.scale
    }

    pub fn set_scale(&mut self, scale: Vec2) {
        self.scale = scale;
    }

    pub fn rotation(&self) -> f32 {
        self.rotation
    }

    pub fn set_rotation(&mut self, rotation: f32) {
        self.rotation = rotation;
    }

    pub fn bounds(&self) -> Rect {
        let position = self.position - self.origin;
        let size = self.frame.size() * self.texture.size().as_vec2();

        Rect::new(position.x, position.y, size.x, size.y)
    }

    pub fn set_bounds(&mut self, rect: Rect) {
        self.set_position(rect.position());
        self.set_size(rect.size());
    }

    pub fn frame(&self) -> Rect {
        Rect::new(
            self.frame.x * self.texture.width() as f32,
            self.frame.y * self.texture.height() as f32,
            self.frame.width * self.texture.width() as f32,
            self.frame.height * self.texture.height() as f32,
        )
    }

    pub fn set_frame(&mut self, frame: Rect) {
        self.frame.x = frame.x / self.texture.width() as f32;
        self.frame.y = frame.y / self.texture.height() as f32;
        self.frame.width = frame.width / self.texture.width() as f32;
        self.frame.height = frame.height / self.texture.height() as f32;
    }

    pub fn color(&self) -> Color {
        self.color
    }

    pub fn set_color(&mut self, color: Color) {
        self.color = color
    }
}

impl Instance<SpriteInstanceData> for Sprite {
    fn instance_data(&self) -> SpriteInstanceData {
        let mut size_scale = self.size();
        size_scale *= self.scale.signum();

        let origin_translation = Mat3::from_translation(-self.origin / size_scale * self.scale);

        let transform =
            Mat3::from_scale_angle_translation(size_scale, self.rotation, self.position)
                * origin_translation;

        SpriteInstanceData {
            transform,
            frame: self.frame,
            color: self.color.to_linear(),
        }
    }

    fn instance_resources(&self) -> Vec<Arc<dyn AsBinding>> {
        vec![self.texture.clone(), self.sampler.clone()]
    }
}

#[repr(C)]
#[derive(Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
pub struct SpriteVertex {
    pub vertex: [f32; 2],
    pub uv: [f32; 2],
}

impl Vertex for SpriteVertex {
    fn vertex_layout() -> VertexLayout {
        VertexLayout::new(&[VertexFormat::Float32x2, VertexFormat::Float32x2])
    }
}

#[repr(C)]
#[derive(Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
pub struct SpriteInstanceData {
    transform: Mat3,
    frame: Rect,
    color: Color,
}

impl InstanceData for SpriteInstanceData {
    fn instance_layout() -> InstanceLayout {
        InstanceLayout::new(&[
            VertexFormat::Float32x3,
            VertexFormat::Float32x3,
            VertexFormat::Float32x3,
            VertexFormat::Float32x4,
            VertexFormat::Float32x4,
        ])
    }
}
