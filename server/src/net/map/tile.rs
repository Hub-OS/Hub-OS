#[derive(PartialEq, Eq, Clone, Default)]
pub struct Tile {
    pub gid: u32,
    pub flipped_horizontally: bool,
    pub flipped_vertically: bool,
    pub flipped_anti_diagonally: bool,
}

impl Tile {
    pub fn from(gid: u32) -> Tile {
        Tile {
            gid: gid << 3 >> 3, // shift off tile properties
            flipped_horizontally: (gid >> 31 & 1) == 1,
            flipped_vertically: (gid >> 30 & 1) == 1,
            flipped_anti_diagonally: (gid >> 29 & 1) == 1,
        }
    }

    pub fn compress(&self) -> u32 {
        self.gid
            | (self.flipped_horizontally as u32) << 31
            | (self.flipped_vertically as u32) << 30
            | (self.flipped_anti_diagonally as u32) << 29
    }
}
