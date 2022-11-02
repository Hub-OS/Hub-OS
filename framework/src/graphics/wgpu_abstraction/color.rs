#[repr(C)]
#[derive(Default, Clone, Copy, PartialEq, Debug, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Color {
    pub r: f32,
    pub g: f32,
    pub b: f32,
    pub a: f32,
}

impl Color {
    pub const BLACK: Color = Color::new(0.0, 0.0, 0.0, 1.0);
    pub const WHITE: Color = Color::new(1.0, 1.0, 1.0, 1.0);
    pub const RED: Color = Color::new(1.0, 0.0, 0.0, 1.0);
    pub const GREEN: Color = Color::new(0.0, 1.0, 0.0, 1.0);
    pub const BLUE: Color = Color::new(0.0, 0.0, 1.0, 1.0);
    pub const YELLOW: Color = Color::new(1.0, 1.0, 0.0, 1.0);
    pub const CYAN: Color = Color::new(0.0, 1.0, 1.0, 1.0);
    pub const MAGENTA: Color = Color::new(1.0, 0.0, 1.0, 1.0);
    pub const ORANGE: Color = Color::new(1.0, 0.5, 0.0, 1.0);
    pub const TRANSPARENT: Color = Color::new(0.0, 0.0, 0.0, 0.0);

    pub const fn new(r: f32, g: f32, b: f32, a: f32) -> Color {
        Color { r, g, b, a }
    }

    pub fn to_linear(mut self) -> Color {
        self.r = to_linear(self.r);
        self.g = to_linear(self.g);
        self.b = to_linear(self.b);

        self
    }

    pub fn to_srgb(mut self) -> Color {
        self.r = to_srgb(self.r);
        self.g = to_srgb(self.g);
        self.b = to_srgb(self.b);

        self
    }

    pub fn multiply_color(mut self, value: f32) -> Self {
        self.r *= value;
        self.g *= value;
        self.b *= value;
        self
    }

    pub fn multiply_alpha(mut self, a: f32) -> Self {
        self.a *= a;
        self
    }

    pub fn lerp(start: Color, end: Color, progress: f32) -> Color {
        let multiplied_start = start * (1.0 - progress);
        let multiplied_end = end * progress;

        Color {
            r: multiplied_end.r + multiplied_start.r,
            g: multiplied_end.g + multiplied_start.g,
            b: multiplied_end.b + multiplied_start.b,
            a: multiplied_end.a + multiplied_start.a,
        }
    }
}

// https://entropymine.com/imageworsener/srgbformula/
fn to_linear(v: f32) -> f32 {
    if v <= 0.04045 {
        v / 12.92
    } else {
        ((v + 0.055) / 1.055).powf(2.4)
    }
}

fn to_srgb(v: f32) -> f32 {
    if v <= 0.0031308 {
        v * 12.92
    } else {
        (1.055 * v.powf(1.0 / 2.4)) - 0.055
    }
}

impl std::ops::Mul<f32> for Color {
    type Output = Self;

    fn mul(mut self, rhs: f32) -> Self {
        self.r *= rhs;
        self.g *= rhs;
        self.b *= rhs;
        self.a *= rhs;
        self
    }
}

impl From<Color> for wgpu::Color {
    fn from(color: Color) -> Self {
        Self {
            r: color.r as f64,
            g: color.g as f64,
            b: color.b as f64,
            a: color.a as f64,
        }
    }
}

impl From<(f32, f32, f32, f32)> for Color {
    fn from(rgba: (f32, f32, f32, f32)) -> Self {
        Self::new(rgba.0, rgba.1, rgba.2, rgba.3)
    }
}

impl From<(f32, f32, f32)> for Color {
    fn from(rgb: (f32, f32, f32)) -> Self {
        Self::new(rgb.0, rgb.1, rgb.2, 1.0)
    }
}

impl From<(u8, u8, u8, u8)> for Color {
    fn from(rgba: (u8, u8, u8, u8)) -> Self {
        Self::new(
            rgba.0 as f32 / 255.0,
            rgba.1 as f32 / 255.0,
            rgba.2 as f32 / 255.0,
            rgba.3 as f32 / 255.0,
        )
    }
}

impl From<(u8, u8, u8)> for Color {
    fn from(rgb: (u8, u8, u8)) -> Self {
        Self::new(
            rgb.0 as f32 / 255.0,
            rgb.1 as f32 / 255.0,
            rgb.2 as f32 / 255.0,
            1.0,
        )
    }
}
