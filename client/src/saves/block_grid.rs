use crate::{
    packages::{BlockPackage, PackageNamespace},
    resources::Globals,
    saves::InstalledBlock,
};
use framework::prelude::GameIO;
use generational_arena::Arena;
use itertools::Itertools;
use std::ops::Range;

pub struct BlockGrid {
    namespace: PackageNamespace,
    blocks: Arena<InstalledBlock>,
    grid: [Option<generational_arena::Index>; Self::SIDE_LEN * Self::SIDE_LEN],
}

impl BlockGrid {
    pub const SIDE_LEN: usize = 7;
    const MAIN_RANGE: Range<usize> = 1..BlockGrid::SIDE_LEN - 1;

    pub fn is_edge((x, y): (usize, usize)) -> bool {
        !Self::MAIN_RANGE.contains(&x) || !Self::MAIN_RANGE.contains(&y)
    }

    pub fn is_corner((x, y): (usize, usize)) -> bool {
        !Self::MAIN_RANGE.contains(&x) && !Self::MAIN_RANGE.contains(&y)
    }

    pub fn new(namespace: PackageNamespace) -> Self {
        Self {
            namespace,
            blocks: Arena::new(),
            grid: [None; Self::SIDE_LEN * Self::SIDE_LEN],
        }
    }

    pub fn with_namespace(mut self, namespace: PackageNamespace) -> Self {
        self.namespace = namespace;
        self
    }

    pub fn with_blocks(mut self, game_io: &GameIO, blocks: Vec<InstalledBlock>) -> Self {
        self.install_blocks(game_io, blocks);
        self
    }

    pub fn count_colors(&self) -> usize {
        let mut colors = Vec::new();

        for (_, block) in &self.blocks {
            if !colors.contains(&block.color) {
                colors.push(block.color);
            }
        }

        colors.len()
    }

    pub fn installed_blocks(&self) -> impl Iterator<Item = &InstalledBlock> {
        self.blocks.iter().map(|(_, block)| block)
    }

    pub fn install_blocks(&mut self, game_io: &GameIO, blocks: Vec<InstalledBlock>) {
        for block in blocks {
            let Some(conflicts) = self.install_block(game_io, block) else {
                continue;
            };

            for conflict in conflicts {
                self.remove_block(conflict);
            }
        }
    }

    /// Failed placement will provide a list of overlapping positions
    pub fn install_block(
        &mut self,
        game_io: &GameIO,
        block: InstalledBlock,
    ) -> Option<Vec<(usize, usize)>> {
        let globals = game_io.resource::<Globals>().unwrap();

        let Some(package) = globals.block_packages.package_or_fallback(self.namespace, &block.package_id) else {
            return Some(Vec::new());
        };

        let rotation = block.rotation;
        let block_x = block.position.0;
        let block_y = block.position.1;

        // validate the placement
        let mut conflicts = Vec::new();
        let mut intersects_main_grid = false;

        for y in 0..5 {
            for x in 0..5 {
                if !package.exists_at(rotation, (x, y)) {
                    continue;
                }

                let grid_temp_x = block_x + x;
                let grid_temp_y = block_y + y;

                if grid_temp_x < 2 || grid_temp_y < 2 {
                    // out of bounds on the left/top side
                    return Some(Vec::new());
                }

                let grid_x = grid_temp_x - 2;
                let grid_y = grid_temp_y - 2;

                if grid_x >= Self::SIDE_LEN || grid_y >= Self::SIDE_LEN {
                    // out of bounds on the right/bottom side
                    return Some(Vec::new());
                }

                let slot = self.grid.get(grid_y * Self::SIDE_LEN + grid_x).unwrap();

                if slot.is_some() {
                    conflicts.push((grid_x, grid_y));
                }

                if Self::is_corner((grid_x, grid_y)) {
                    // reject placement on corners
                    return Some(Vec::new());
                }

                if !Self::is_edge((grid_x, grid_y)) {
                    intersects_main_grid = true;
                }
            }
        }

        if !intersects_main_grid {
            return Some(Vec::new());
        }

        if !conflicts.is_empty() {
            return Some(conflicts);
        }

        // actual placement
        let block_index = self.blocks.insert(block);

        for y in 0..5 {
            for x in 0..5 {
                if !package.exists_at(rotation, (x, y)) {
                    continue;
                }

                let grid_x = block_x + x - 2;
                let grid_y = block_y + y - 2;
                self.grid[grid_y * Self::SIDE_LEN + grid_x] = Some(block_index);
            }
        }

        None
    }

    pub fn remove_block(&mut self, position: (usize, usize)) -> Option<InstalledBlock> {
        let grid_index = position.1 * Self::SIDE_LEN + position.0;
        let slot = *self.grid.get(grid_index)?;
        let block_index = slot?;

        // clear slots with this block
        for i in 0..self.grid.len() {
            if self.grid[i] == slot {
                self.grid[i] = None;
            }
        }

        self.blocks.remove(block_index)
    }

    pub fn get_block(&self, position: (usize, usize)) -> Option<&InstalledBlock> {
        let grid_index = position.1 * Self::SIDE_LEN + position.0;
        let slot = *self.grid.get(grid_index)?;
        let block_index = slot?;

        self.blocks.get(block_index)
    }

    pub fn installed_packages<'a>(
        &'a self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = &'a BlockPackage> {
        let globals = game_io.resource::<Globals>().unwrap();

        self.blocks
            .iter()
            .map(|(_, block)| block)
            .flat_map(|block| {
                globals
                    .block_packages
                    .package_or_fallback(self.namespace, &block.package_id)
            })
    }

    fn valid_flat_packages<'a>(
        &'a self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = &'a BlockPackage> {
        let globals = game_io.resource::<Globals>().unwrap();

        (0..Self::SIDE_LEN)
            .flat_map(|x| self.get_block((x, 3)))
            .unique_by(|block| block.position)
            .flat_map(|block| {
                globals
                    .block_packages
                    .package_or_fallback(self.namespace, &block.package_id)
            })
            .filter(|package| package.is_program)
    }

    fn valid_plus_packages<'a>(
        &'a self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = &'a BlockPackage> {
        self.installed_packages(game_io)
            .filter(|package| !package.is_program)
    }

    pub fn valid_packages<'a>(
        &'a self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = &'a BlockPackage> {
        self.valid_flat_packages(game_io)
            .chain(self.valid_plus_packages(game_io))
    }
}
