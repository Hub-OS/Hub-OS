use crate::prelude::*;
use std::cell::RefCell;

const OPENGL_TO_WGPU_MATRIX: Mat4 = Mat4::from_cols_array_2d(&[
    [1.0, 0.0, 0.0, 0.0],
    [0.0, 1.0, 0.0, 0.0],
    [0.0, 0.0, 0.5, 0.0],
    [0.0, 0.0, 0.5, 1.0],
]);

#[derive(Clone, Copy, PartialEq)]
struct CameraState {
    translation: Vec3,
    invert_y: f32,
    width: f32,
    height: f32,
    requested_width: f32,
    requested_height: f32,
    scale: Vec2,
}

impl CameraState {
    fn create_matrix(&self) -> Mat4 {
        let half_width = self.width * 0.5;
        let half_height = self.height * 0.5;

        let mut view_projection = Mat4::orthographic_rh(
            -half_width,                  // left
            half_width,                   // right
            -half_height * self.invert_y, // bottom
            half_height * self.invert_y,  // top
            -10000.0,
            10000.0,
        );

        // scale
        let width_ratio = self.width / self.requested_width;
        let height_ratio = self.height / self.requested_height;

        let scale_correction = if width_ratio < height_ratio {
            self.width / self.requested_width
        } else {
            self.height / self.requested_height
        };

        let scale = self.scale * scale_correction;
        view_projection *= Mat4::from_scale(Vec3::new(scale.x, scale.y, 1.0));

        // translate
        view_projection *= Mat4::from_translation(-self.translation);

        let final_matrix = OPENGL_TO_WGPU_MATRIX * view_projection;

        final_matrix
    }
}

pub struct OrthoCamera {
    state: CameraState,
    prev_state: RefCell<CameraState>,
    buffer_resource: BufferResource,
}

impl OrthoCamera {
    pub fn new<Globals>(game_io: &GameIO<Globals>, view_size: Vec2) -> OrthoCamera {
        let state = CameraState {
            translation: Vec3::ZERO,
            invert_y: 1.0,
            width: view_size.x,
            height: view_size.y,
            requested_width: view_size.x,
            requested_height: view_size.y,
            scale: Vec2::new(1.0, 1.0),
        };

        let buffer_resource =
            BufferResource::new(game_io, bytemuck::cast_slice(&[state.create_matrix()]));

        OrthoCamera {
            state,
            prev_state: RefCell::new(state),
            buffer_resource,
        }
    }

    pub fn is_y_inverted(&mut self) -> bool {
        self.state.invert_y < 0.0
    }

    pub fn invert_y(&mut self, invert: bool) {
        self.state.invert_y = if invert { -1.0 } else { 1.0 };
    }

    pub fn x(&self) -> f32 {
        self.state.translation.x
    }

    pub fn set_x(&mut self, x: f32) {
        self.state.translation.x = x
    }

    pub fn y(&self) -> f32 {
        self.state.translation.y
    }

    pub fn set_y(&mut self, y: f32) {
        self.state.translation.y = y
    }

    pub fn z(&self) -> f32 {
        self.state.translation.z
    }

    pub fn set_z(&mut self, z: f32) {
        self.state.translation.z = z
    }

    pub fn position(&self) -> Vec3 {
        self.state.translation
    }

    pub fn set_position(&mut self, position: Vec3) {
        self.state.translation = position;
    }

    pub fn resize(&mut self, size: Vec2) {
        self.state.width = size.x;
        self.state.height = size.y;
        self.state.requested_width = size.x;
        self.state.requested_height = size.y;
    }

    pub fn resize_to_window(&mut self, window: &Window) {
        let resolution = window.resolution();

        self.state.width = resolution.x as f32;
        self.state.height = resolution.y as f32;
        self.state.requested_width = self.state.width;
        self.state.requested_height = self.state.height;
    }

    pub fn scale(&mut self, scale: Vec2) {
        self.state.scale = scale;
    }

    pub fn scale_with_window(&mut self, window: &Window) {
        let res = window.resolution();

        self.state.width = res.x as f32;
        self.state.height = res.y as f32;
    }

    pub fn size(&self) -> Vec2 {
        Vec2::new(self.state.height, self.state.width)
    }

    pub fn bounds(&self) -> Rect {
        Rect::new(
            self.state.translation.x - self.state.width * 0.5,
            self.state.translation.y - self.state.height * 0.5,
            self.state.width,
            self.state.height,
        )
    }

    pub fn bind_group_layout_entry(binding: u32) -> wgpu::BindGroupLayoutEntry {
        wgpu::BindGroupLayoutEntry {
            binding,
            visibility: wgpu::ShaderStages::VERTEX,
            ty: wgpu::BindingType::Buffer {
                ty: wgpu::BufferBindingType::Uniform,
                has_dynamic_offset: false,
                min_binding_size: None,
            },
            count: None,
        }
    }
}

impl AsBinding for OrthoCamera {
    fn as_binding(&self) -> BindingResource<'_> {
        let mut prev_state = self.prev_state.borrow_mut();

        if self.state != *prev_state {
            self.buffer_resource
                .write(0, bytemuck::cast_slice(&[self.state.create_matrix()]));

            *prev_state = self.state;
        }

        self.buffer_resource.as_binding()
    }
}
