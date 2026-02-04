#[derive(Default, Clone, Copy)]
pub struct BlockShape(u64);

impl BlockShape {
    pub const CENTER_OFFSET: i8 = 3;

    pub fn exists_at(&self, rotation: u8, position: (i8, i8)) -> bool {
        if !(0..=6).contains(&position.0) || !(0..=6).contains(&position.1) {
            return false;
        }

        let position = (position.0 as u8, position.1 as u8);

        let (x, y) = match rotation {
            1 => (position.1, 6 - position.0),
            2 => (6 - position.0, 6 - position.1),
            3 => (6 - position.1, position.0),
            _ => (position.0, position.1),
        };

        (self.0 >> (y * 8 + x)) & 1 == 1
    }
}

pub struct InvalidBlockShape;

impl std::fmt::Display for InvalidBlockShape {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Block shapes must be provided as a 7x7 max rectangle (7 lists of 7 numbers)"
        )
    }
}

impl TryFrom<Vec<Vec<u8>>> for BlockShape {
    type Error = InvalidBlockShape;

    fn try_from(lines: Vec<Vec<u8>>) -> Result<Self, Self::Error> {
        let mut data = 0u64;

        if lines.len() > 7 {
            return Err(InvalidBlockShape);
        }

        // offsets to center the block to match what the author might've intended
        let width = lines.first().map(|line| line.len()).unwrap_or_default();

        if width > 7 {
            return Err(InvalidBlockShape);
        }

        let x_offset = (7 - width).div_ceil(2);
        let y_offset = (7 - lines.len()).div_ceil(2);

        for (y, line) in lines.into_iter().enumerate() {
            for (x, value) in line.into_iter().enumerate() {
                if x >= width {
                    return Err(InvalidBlockShape);
                }

                if value != 0 {
                    data |= 1 << ((y + y_offset) * 8 + x + x_offset);
                }
            }
        }

        Ok(Self(data))
    }
}
