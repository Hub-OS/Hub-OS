pub trait Shape: Send + Sync {
    fn x(&self) -> f32;
    fn y(&self) -> f32;
    fn width(&self) -> f32;
    fn height(&self) -> f32;
    fn rotation(&self) -> f32;

    // point is within the shape or on an edge
    fn intersects(&self, point: (f32, f32)) -> bool;

    fn rotate_around(&self, point: &mut (f32, f32)) {
        if self.rotation() != 0.0 {
            let relative_x = point.0 - self.x();
            let relative_y = point.1 - self.y();

            let distance = relative_x.hypot(relative_y).sqrt();
            let relative_radians = relative_y.atan2(relative_x);
            let rotation_radians = (self.rotation() / 180.0).to_radians();
            let radians = relative_radians + rotation_radians;

            // set x + y to a position rotated around self.x() + self.y()
            point.0 = radians.cos() * distance + self.x();
            point.1 = radians.sin() * distance + self.y();
        }
    }
}
