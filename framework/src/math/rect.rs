use super::*;
use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Range, Sub, SubAssign};

macro_rules! impl_rect2 {
    ($name: ident, $vec2: ident,  $type: ty, $zero: literal, $one: literal) => {
        #[repr(C)]
        #[derive(Debug, Default, Clone, Copy, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
        pub struct $name {
            pub x: $type,
            pub y: $type,
            pub width: $type,
            pub height: $type,
        }

        impl $name {
            pub const ZERO: $name = $name::new($zero, $zero, $zero, $zero);
            pub const UNIT: $name = $name::new($zero, $zero, $one, $one);

            pub const fn new(x: $type, y: $type, width: $type, height: $type) -> Self {
                Self {
                    x,
                    y,
                    width,
                    height,
                }
            }

            pub fn from_corners(top_left: $vec2, bottom_right: $vec2) -> Self {
                Self {
                    x: top_left.x,
                    y: top_left.y,
                    width: bottom_right.x - top_left.x,
                    height: bottom_right.y - top_left.y,
                }
            }

            pub fn left(&self) -> $type {
                (self.x).min(self.x + self.width)
            }

            pub fn right(&self) -> $type {
                (self.x).max(self.x + self.width)
            }

            pub fn top(&self) -> $type {
                (self.y).min(self.y + self.height)
            }

            pub fn bottom(&self) -> $type {
                (self.y).max(self.y + self.height)
            }

            pub fn center_left(&self) -> $vec2 {
                $vec2::new(self.left(), (self.top() + self.bottom()) / ($one + $one))
            }

            pub fn top_left(&self) -> $vec2 {
                $vec2::new(self.left(), self.top())
            }

            pub fn top_right(&self) -> $vec2 {
                $vec2::new(self.right(), self.top())
            }

            pub fn center_right(&self) -> $vec2 {
                $vec2::new(self.right(), (self.top() + self.bottom()) / ($one + $one))
            }

            pub fn bottom_left(&self) -> $vec2 {
                $vec2::new(self.left(), self.bottom())
            }

            pub fn bottom_right(&self) -> $vec2 {
                $vec2::new(self.right(), self.bottom())
            }

            /// Same as self.left()..self.right()
            pub fn horizontal_range(&self) -> Range<$type> {
                self.left()..self.right()
            }

            /// Same as self.top()..self.bottom()
            pub fn vertical_range(&self) -> Range<$type> {
                self.top()..self.bottom()
            }

            pub const fn position(&self) -> $vec2 {
                $vec2 {
                    x: self.x,
                    y: self.y,
                }
            }

            pub fn set_position(&mut self, position: $vec2) {
                self.x = position.x;
                self.y = position.y;
            }

            pub const fn size(&self) -> $vec2 {
                $vec2 {
                    x: self.width,
                    y: self.height,
                }
            }

            pub fn set_size(&mut self, size: $vec2) {
                self.width = size.x;
                self.height = size.y;
            }

            /// Excludes the bottom and right edges
            pub fn contains(&self, point: $vec2) -> bool {
                self.horizontal_range().contains(&point.x)
                    && self.vertical_range().contains(&point.y)
            }

            /// Excludes the bottom and right edges
            pub fn overlaps(&self, other: Self) -> bool {
                let contains_x = self.horizontal_range().contains(&other.left())
                    || self.horizontal_range().contains(&self.left());

                let contains_y = self.vertical_range().contains(&other.top())
                    || other.vertical_range().contains(&self.top());

                contains_x && contains_y
            }

            /// Returns the overlapping region of the two rectangles
            pub fn scissor(&self, bounds: Self) -> Self {
                #![allow(unused_comparisons)]

                let (min_x, max_x, min_y, max_y);

                if bounds.width >= $zero {
                    min_x = bounds.x;
                    max_x = bounds.x + bounds.width;
                } else {
                    min_x = bounds.x + bounds.width;
                    max_x = bounds.x;
                }

                if bounds.height >= $zero {
                    min_y = bounds.y;
                    max_y = bounds.y + bounds.height;
                } else {
                    min_y = bounds.y + bounds.height;
                    max_y = bounds.y;
                }

                let x = self.x.clamp(min_x, max_x);
                let y = self.y.clamp(min_y, max_y);
                let width = (x + self.width).clamp(min_x, max_x) - x;
                let height = (y + self.height).clamp(min_y, max_y) - y;

                Self {
                    x,
                    y,
                    width,
                    height,
                }
            }
        }

        impl From<($type, $type, $type, $type)> for $name {
            fn from((x, y, width, height): ($type, $type, $type, $type)) -> Self {
                $name {
                    x,
                    y,
                    width,
                    height,
                }
            }
        }

        impl From<$name> for ($type, $type, $type, $type) {
            fn from(rect: $name) -> Self {
                (rect.x, rect.y, rect.width, rect.height)
            }
        }

        // impl From<(($type, $type), ($type, $type))> for $name {
        //     fn from((position, size): (($type, $type), ($type, $type))) -> Self {
        //         $name {
        //             x: position.0,
        //             y: position.1,
        //             width: size.0,
        //             height: size.1,
        //         }
        //     }
        // }

        // impl From<($vec2, $vec2)> for $name {
        //     fn from((position, size): ($vec2, $vec2)) -> Self {
        //         $name {
        //             x: position.x,
        //             y: position.y,
        //             width: size.x,
        //             height: size.y,
        //         }
        //     }
        // }

        impl From<[$type; 4]> for $name {
            fn from([x, y, width, height]: [$type; 4]) -> Self {
                $name {
                    x,
                    y,
                    width,
                    height,
                }
            }
        }

        impl From<$name> for [$type; 4] {
            fn from(rect: $name) -> Self {
                [rect.x, rect.y, rect.width, rect.height]
            }
        }

        impl Add<$vec2> for $name {
            type Output = $name;

            fn add(mut self, rhs: $vec2) -> Self::Output {
                self.x += rhs.x;
                self.y += rhs.y;

                self
            }
        }

        impl Add<$name> for $vec2 {
            type Output = $name;

            fn add(self, mut rhs: $name) -> Self::Output {
                rhs.x += self.x;
                rhs.y += self.y;

                rhs
            }
        }

        impl AddAssign<$vec2> for $name {
            fn add_assign(&mut self, rhs: $vec2) {
                self.x += rhs.x;
                self.y += rhs.y;
            }
        }

        impl Sub<$vec2> for $name {
            type Output = $name;

            fn sub(mut self, rhs: $vec2) -> Self::Output {
                self.x -= rhs.x;
                self.y -= rhs.y;

                self
            }
        }

        impl SubAssign<$vec2> for $name {
            fn sub_assign(&mut self, rhs: $vec2) {
                self.x -= rhs.x;
                self.y -= rhs.y;
            }
        }

        impl Mul<$type> for $name {
            type Output = $name;

            fn mul(mut self, rhs: $type) -> Self::Output {
                self.x *= rhs;
                self.y *= rhs;
                self.width *= rhs;
                self.height *= rhs;

                self
            }
        }

        impl Mul<$name> for $type {
            type Output = $name;

            fn mul(self, mut rhs: $name) -> Self::Output {
                rhs.x *= self;
                rhs.y *= self;
                rhs.width *= self;
                rhs.height *= self;

                rhs
            }
        }

        impl MulAssign<$type> for $name {
            fn mul_assign(&mut self, rhs: $type) {
                self.x *= rhs;
                self.y *= rhs;
                self.width *= rhs;
                self.height *= rhs;
            }
        }

        impl Mul<$vec2> for $name {
            type Output = $name;

            fn mul(mut self, rhs: $vec2) -> Self::Output {
                self.x *= rhs.x;
                self.y *= rhs.y;
                self.width *= rhs.x;
                self.height *= rhs.y;

                self
            }
        }

        impl Mul<$name> for $vec2 {
            type Output = $name;

            fn mul(self, mut rhs: $name) -> Self::Output {
                rhs.x *= self.x;
                rhs.y *= self.y;
                rhs.width *= self.x;
                rhs.height *= self.y;

                rhs
            }
        }

        impl MulAssign<$vec2> for $name {
            fn mul_assign(&mut self, rhs: $vec2) {
                self.x *= rhs.x;
                self.y *= rhs.y;
                self.width *= rhs.x;
                self.height *= rhs.y;
            }
        }

        impl Div<$type> for $name {
            type Output = $name;

            fn div(mut self, rhs: $type) -> Self::Output {
                self.x /= rhs;
                self.y /= rhs;
                self.width /= rhs;
                self.height /= rhs;

                self
            }
        }

        impl Div<$name> for $type {
            type Output = $name;

            fn div(self, mut rhs: $name) -> Self::Output {
                rhs.x /= self;
                rhs.y /= self;
                rhs.width /= self;
                rhs.height /= self;

                rhs
            }
        }

        impl DivAssign<$type> for $name {
            fn div_assign(&mut self, rhs: $type) {
                self.x /= rhs;
                self.y /= rhs;
                self.width /= rhs;
                self.height /= rhs;
            }
        }

        impl Div<$vec2> for $name {
            type Output = $name;

            fn div(mut self, rhs: $vec2) -> Self::Output {
                self.x /= rhs.x;
                self.y /= rhs.y;
                self.width /= rhs.x;
                self.height /= rhs.y;

                self
            }
        }

        impl Div<$name> for $vec2 {
            type Output = $name;

            fn div(self, mut rhs: $name) -> Self::Output {
                rhs.x /= self.x;
                rhs.y /= self.y;
                rhs.width /= self.x;
                rhs.height /= self.y;

                rhs
            }
        }

        impl DivAssign<$vec2> for $name {
            fn div_assign(&mut self, rhs: $vec2) {
                self.x /= rhs.x;
                self.y /= rhs.y;
                self.width /= rhs.x;
                self.height /= rhs.y;
            }
        }
    };
}

impl_rect2!(Rect, Vec2, f32, 0.0, 1.0);
impl_rect2!(DRect, DVec2, f64, 0.0, 1.0);
impl_rect2!(IRect, IVec2, i32, 0, 1);
impl_rect2!(URect, UVec2, u32, 0, 1);

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn overlaps() {
        const TOP_LEFT: Rect = Rect::new(-0.5, -0.5, 1.0, 1.0);
        const LARGER: Rect = Rect::new(-1.0, -1.0, 1.5, 1.5);
        const NEGATIVE: Rect = Rect::new(1.0, 1.0, -0.5, -0.5);
        const BOTTOM_RIGHT_CORNER: Rect = Rect::new(1.0, 1.0, 1.0, 1.0);
        const OUTSIDE: Rect = Rect::new(2.0, 1.0, 1.0, 1.0);

        assert!(Rect::UNIT.overlaps(TOP_LEFT));
        assert!(TOP_LEFT.overlaps(Rect::UNIT));

        assert!(Rect::UNIT.overlaps(LARGER));
        assert!(LARGER.overlaps(Rect::UNIT));

        assert!(Rect::UNIT.overlaps(NEGATIVE));
        assert!(NEGATIVE.overlaps(Rect::UNIT));

        assert!(!Rect::UNIT.overlaps(BOTTOM_RIGHT_CORNER));
        assert!(!BOTTOM_RIGHT_CORNER.overlaps(Rect::UNIT));

        assert!(!Rect::UNIT.overlaps(OUTSIDE));
        assert!(!OUTSIDE.overlaps(Rect::UNIT));
    }
}
