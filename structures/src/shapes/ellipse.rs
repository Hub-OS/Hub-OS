use super::Shape;

pub struct Ellipse {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    rotation: f32,
}

impl Ellipse {
    pub fn new(x: f32, y: f32, width: f32, height: f32, rotation: f32) -> Ellipse {
        Ellipse {
            x,
            y,
            width,
            height,
            rotation,
        }
    }
}

impl Shape for Ellipse {
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

        // add half size as tiled centers ellipse at the top left
        let distance_x = x - (self.x + self.width * 0.5);
        let distance_y = y - (self.y + self.height * 0.5);

        let radius_x = self.width * 0.5;
        let radius_y = self.height * 0.5;

        // convert distances to relative positions on a unit circle
        let unit_x = distance_x / radius_x;
        let unit_y = distance_y / radius_y;

        // unit circle has a radius of 1
        // so we can just test for <= 1 to see if this point is within the circle
        unit_x * unit_x + unit_y * unit_y <= 1.0
    }
}
