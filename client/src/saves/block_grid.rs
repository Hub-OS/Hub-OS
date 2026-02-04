use crate::packages::{AugmentPackage, PackageNamespace};
use crate::resources::Globals;
use crate::saves::{BlockShape, InstalledBlock};
use crate::structures::{GenerationalIndex, SlotMap};
use framework::prelude::GameIO;
use packets::structures::{PackageCategory, PackageId};
use std::ops::Range;

pub struct BlockGrid {
    namespace: PackageNamespace,
    blocks: SlotMap<InstalledBlock>,
    grid: [Option<GenerationalIndex>; Self::SIDE_LEN as usize * Self::SIDE_LEN as usize],
}

impl BlockGrid {
    pub const LINE_Y: i8 = 3;
    pub const SIDE_LEN: i8 = 7;
    const MAIN_RANGE: Range<i8> = 1..BlockGrid::SIDE_LEN - 1;

    pub fn is_edge((x, y): (i8, i8)) -> bool {
        !Self::MAIN_RANGE.contains(&x) || !Self::MAIN_RANGE.contains(&y)
    }

    pub fn is_corner((x, y): (i8, i8)) -> bool {
        !Self::MAIN_RANGE.contains(&x) && !Self::MAIN_RANGE.contains(&y)
    }

    pub fn new(namespace: PackageNamespace) -> Self {
        Self {
            namespace,
            blocks: Default::default(),
            grid: [None; _],
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
    ) -> Option<Vec<(i8, i8)>> {
        let globals = Globals::from_resources(game_io);

        let Some(package) = globals
            .augment_packages
            .package_or_fallback(self.namespace, &block.package_id)
        else {
            return Some(Vec::new());
        };

        let rotation = block.rotation;
        let block_x = block.position.0;
        let block_y = block.position.1;

        // validate the placement
        let mut conflicts = Vec::new();
        let mut intersects_main_grid = false;

        for y in 0..Self::SIDE_LEN {
            for x in 0..Self::SIDE_LEN {
                if !package.exists_at(block.variant, rotation, (x, y)) {
                    continue;
                }

                let grid_temp_x = block_x + x;
                let grid_temp_y = block_y + y;

                if grid_temp_x < BlockShape::CENTER_OFFSET
                    || grid_temp_y < BlockShape::CENTER_OFFSET
                {
                    // out of bounds on the left/top side
                    return Some(Vec::new());
                }

                let grid_x = grid_temp_x - BlockShape::CENTER_OFFSET;
                let grid_y = grid_temp_y - BlockShape::CENTER_OFFSET;

                if grid_x >= Self::SIDE_LEN || grid_y >= Self::SIDE_LEN {
                    // out of bounds on the right/bottom side
                    return Some(Vec::new());
                }

                let index = Self::calculate_index((grid_x, grid_y));
                let slot = self.grid.get(index).unwrap();

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
        self.blocks.insert_with_key(|block_index| {
            for position in Self::iterate_block_positions(&block, package) {
                self.grid[Self::calculate_index(position)] = Some(block_index);
            }

            block
        });

        None
    }

    pub fn iterate_block_positions<'a>(
        block: &'a InstalledBlock,
        package: &'a AugmentPackage,
    ) -> impl Iterator<Item = (i8, i8)> + 'a {
        (0..BlockGrid::SIDE_LEN)
            .flat_map(|y| (0..BlockGrid::SIDE_LEN).map(move |x| (x, y)))
            .filter(|&position| package.exists_at(block.variant, block.rotation, position))
            .map(|(x, y)| {
                (
                    block.position.0 + x - BlockShape::CENTER_OFFSET,
                    block.position.1 + y - BlockShape::CENTER_OFFSET,
                )
            })
    }

    pub fn remove_block(&mut self, position: (i8, i8)) -> Option<InstalledBlock> {
        let grid_index = Self::calculate_index(position);
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

    pub fn get_block(&self, position: (i8, i8)) -> Option<&InstalledBlock> {
        let grid_index = Self::calculate_index(position);
        let slot = *self.grid.get(grid_index)?;
        let block_index = slot?;

        self.blocks.get(block_index)
    }

    fn calculate_index(position: (i8, i8)) -> usize {
        position.1 as usize * Self::SIDE_LEN as usize + position.0 as usize
    }

    pub fn installed_packages<'a>(
        &'a self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = &'a AugmentPackage> {
        let globals = Globals::from_resources(game_io);

        self.blocks
            .iter()
            .map(|(_, block)| block)
            .flat_map(|block| {
                globals
                    .augment_packages
                    .package_or_fallback(self.namespace, &block.package_id)
            })
    }

    pub fn augments<'a>(
        &self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = (&'a AugmentPackage, usize)> + use<'a> {
        let globals = Globals::from_resources(game_io);
        let augment_packages = &globals.augment_packages;

        let mut list = Vec::new();
        let mut byproducts = Vec::new();
        let mut prevent_byproducts = false;

        // special insert, adds to count + preserves order
        let push_id =
            |list: &mut Vec<(&'a PackageId, usize, bool)>, id: &'a PackageId, priority: bool| {
                if let Some(index) = list.iter().position(|(stored, _, _)| *stored == id) {
                    list[index].1 += 1;
                    list[index].2 |= priority;
                } else {
                    list.push((id, 1, priority));
                }
            };

        for block in self.blocks.values() {
            let Some(package) =
                augment_packages.package_or_fallback(self.namespace, &block.package_id)
            else {
                continue;
            };

            let touches_line = Self::touches_line(block, package);

            if !package.is_flat || touches_line {
                // add augment if it's valid
                push_id(&mut list, &package.package_info.id, false);
                prevent_byproducts |= package.prevent_byproducts;
            }

            if (!package.is_flat && touches_line) || self.has_conflicts(block, package) {
                // append byproducts if we're invalid

                // filter out byproducts that aren't in the dependency list
                let package_requirements = &package.package_info.requirements;
                let new_byproduct_iter = package.byproducts.iter().filter(|&id| {
                    let is_dependency = package_requirements.iter().any(|(r_category, r_id)| {
                        *r_category == PackageCategory::Augment && id == r_id
                    });

                    if !is_dependency {
                        log::error!(
                            "Ignoring byproduct {id} missing from dependencies in {}",
                            block.package_id
                        );
                    }

                    is_dependency
                });

                byproducts.extend(new_byproduct_iter);
            }
        }

        if !prevent_byproducts {
            // add byproducts as priority to override other augments
            for id in byproducts {
                push_id(&mut list, id, true);
            }
        }

        // sort by priority, putting low priority first
        list.sort_by_key(|(_, _, priority)| *priority);

        // iterate in reverse to put priority first
        let namespace = self.namespace;

        list.into_iter().rev().flat_map(move |(id, count, _)| {
            Some((augment_packages.package_or_fallback(namespace, id)?, count))
        })
    }

    fn touches_line(block: &InstalledBlock, package: &AugmentPackage) -> bool {
        (0..BlockGrid::SIDE_LEN).any(|x| {
            package.exists_at(
                block.variant,
                block.rotation,
                (
                    x + BlockShape::CENTER_OFFSET - block.position.0,
                    BlockGrid::LINE_Y + BlockShape::CENTER_OFFSET - block.position.1,
                ),
            )
        })
    }

    fn has_conflicts(&self, block: &InstalledBlock, package: &AugmentPackage) -> bool {
        Self::iterate_block_positions(block, package)
            .any(|position| Self::is_edge(position) || self.has_conflicting_neighbors(position))
    }

    fn has_conflicting_neighbors(&self, position: (i8, i8)) -> bool {
        let grid_index = Self::calculate_index(position);
        let Some(reference) = self.grid[grid_index] else {
            return false;
        };

        let reference_color = self.blocks[reference].color;
        self.neighbor_indices(position)
            .any(|index| index != reference && self.blocks[index].color == reference_color)
    }

    // skips edges
    fn neighbor_indices(&self, position: (i8, i8)) -> impl Iterator<Item = GenerationalIndex> + '_ {
        let (x, y) = position;
        let mut neighbors = [None; 4];

        if y > 1 {
            let grid_index = Self::calculate_index((x, y - 1));
            neighbors[0] = self.grid.get(grid_index).cloned().flatten();
        }

        if x > 1 {
            let grid_index = Self::calculate_index((x - 1, y));
            neighbors[1] = self.grid.get(grid_index).cloned().flatten();
        }

        if x + 1 < Self::SIDE_LEN {
            let grid_index = Self::calculate_index((x + 1, y));
            neighbors[2] = self.grid.get(grid_index).cloned().flatten();
        }

        if y + 1 < Self::SIDE_LEN {
            let grid_index = Self::calculate_index((x, y + 1));
            neighbors[3] = self.grid.get(grid_index).cloned().flatten();
        }

        neighbors.into_iter().flatten()
    }
}
