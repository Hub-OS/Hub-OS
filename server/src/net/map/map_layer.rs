use super::Tile;

#[derive(Clone)]
pub struct MapLayer {
    id: u32,
    name: String,
    data: Vec<u32>, // row * width + col
    width: usize,
    height: usize,
    visible: bool,
    cached: bool,
    cached_string: String,
}

impl MapLayer {
    pub fn new(id: u32, name: String, width: usize, height: usize, data: Vec<u32>) -> MapLayer {
        MapLayer {
            id,
            name,
            data,
            width,
            height,
            visible: true,
            cached: false,
            cached_string: String::new(),
        }
    }

    #[allow(dead_code)]
    pub fn is_visible(&self) -> bool {
        self.visible
    }

    pub fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }

    pub fn get_tile(&self, x: usize, y: usize) -> Tile {
        let raw = self.data[y * self.width + x];

        Tile::from(raw)
    }

    pub fn set_tile(&mut self, x: usize, y: usize, tile: Tile) {
        let idx = y * self.width + x;

        let compact_tile_data = tile.compress();

        if self.data[idx] != compact_tile_data {
            self.data[idx] = compact_tile_data;
            self.cached = false;
        }
    }

    pub fn render(&mut self) -> String {
        if !self.cached {
            let visible_str = if !self.visible { " visible=\"0\"" } else { "" };

            let csv = self
                .data
                .chunks(self.width)
                .map(|row| {
                    row.iter()
                        .map(|gid| gid.to_string())
                        .collect::<Vec<String>>()
                        .join(",")
                })
                .collect::<Vec<String>>()
                .join(",\n");

            self.cached_string = format!(
                "\
          <layer id=\"{}\" name={:?} width=\"{}\" height=\"{}\"{}>\
            <data encoding=\"csv\">{}</data>\
          </layer>\
        ",
                self.id, self.name, self.width, self.height, visible_str, csv
            );
            self.cached = true;
        }

        self.cached_string.clone()
    }
}
