use super::Shape;

pub struct Polygon {
    x: f32,
    y: f32,
    rotation: f32,
    points: Vec<(f32, f32)>,
    width: f32,
    height: f32,
    smallest_x: f32,
    smallest_y: f32,
    largest_x: f32,
    largest_y: f32,
}

impl Polygon {
    pub fn new(x: f32, y: f32, rotation: f32) -> Polygon {
        Polygon {
            x,
            y,
            rotation,
            points: Vec::new(),
            width: 0.0,
            height: 0.0,
            smallest_x: 0.0,
            smallest_y: 0.0,
            largest_x: 0.0,
            largest_y: 0.0,
        }
    }

    pub fn add_point(&mut self, point: (f32, f32)) {
        self.points.push(point);
        let (x, y) = point;

        if self.points.len() == 1 {
            self.smallest_x = x;
            self.smallest_y = y;
            self.largest_x = x;
            self.largest_y = y;
            return;
        }

        if x < self.smallest_x {
            self.smallest_x = x;
        }

        if x > self.largest_x {
            self.largest_x = x;
        }

        if y < self.smallest_y {
            self.smallest_y = y;
        }

        if y > self.largest_y {
            self.largest_y = y;
        }

        self.width = self.largest_x - self.smallest_x;
        self.height = self.largest_y - self.smallest_y;
    }
}

impl Shape for Polygon {
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

    // shoot a ray to the right and count edge intersections
    // even = outside, odd = inside
    fn intersects(&self, mut point: (f32, f32)) -> bool {
        let mut last_point: (f32, f32) = match self.points.last() {
            Some(last_point) => *last_point,
            _ => return false,
        };

        self.rotate_around(&mut point);
        let (x, y) = point;

        let mut intersections = 0;

        for point in &self.points {
            // note: converting to doubles for math to avoid precision issues
            let ax = last_point.0 + self.x;
            let ay = last_point.1 + self.y;
            let bx = point.0 + self.x;
            let by = point.1 + self.y;
            last_point = *point;

            let run: f64 = (bx - ax) as f64;

            // make sure y is between these points
            // excluding the top point (avoid colliding with vertex twice)
            let y_is_within = (y >= ay && y < by) || (y >= by && y < ay);

            if !y_is_within {
                continue;
            }

            if run == 0.0 {
                // column line
                if x < ax {
                    // column is to the right and y is within
                    intersections += 1;
                }
            } else {
                // y = slope * x + y_intercept
                let rise: f64 = (by - ay) as f64;
                let slope: f64 = rise / run;
                let y_intercept: f64 = ay as f64 - slope * ax as f64;

                // find an intersection at y
                // converting back to float for final comparison
                // algebra: x = (y - y_intercept) / slope
                let intersection_x = ((y as f64 - y_intercept) / slope) as f32;

                // make sure x is between these points
                let x_is_within = (intersection_x >= ax && intersection_x <= bx)
                    || (intersection_x >= bx && intersection_x <= ax);

                // make sure intersection_x is to the right
                if x_is_within && x <= intersection_x {
                    intersections += 1;
                }
            }
        }

        // odd = inside
        intersections % 2 == 1
    }
}
