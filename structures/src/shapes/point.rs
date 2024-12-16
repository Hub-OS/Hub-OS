use super::Shape;

pub struct Point {
    x: f32,
    y: f32,
}

impl Point {
    pub fn new(x: f32, y: f32) -> Point {
        Point { x, y }
    }
}

impl Shape for Point {
    fn x(&self) -> f32 {
        self.x
    }

    fn y(&self) -> f32 {
        self.y
    }

    fn width(&self) -> f32 {
        0.0
    }

    fn height(&self) -> f32 {
        0.0
    }

    fn rotation(&self) -> f32 {
        0.0
    }

    fn intersects(&self, _point: (f32, f32)) -> bool {
        // points have no size, no possible intersection
        false
    }
}
