use super::Shape;

pub struct Rect {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    rotation: f32,
}

impl Rect {
    pub fn new(x: f32, y: f32, width: f32, height: f32, rotation: f32) -> Rect {
        Rect {
            x,
            y,
            width,
            height,
            rotation,
        }
    }
}

impl Shape for Rect {
    fn x(&self) -> f32 {
        self.x
    }

    fn y(&self) -> f32 {
        self.y
    }

    fn width(&self) -> f32 {
        self.width
    }

    fn height(&self) -> f32 {
        self.height
    }

    fn rotation(&self) -> f32 {
        self.rotation
    }

    fn intersects(&self, mut point: (f32, f32)) -> bool {
        self.rotate_around(&mut point);
        let (x, y) = point;

        x >= self.x && y >= self.y && x <= self.x + self.width && y <= self.y + self.height
    }
}
