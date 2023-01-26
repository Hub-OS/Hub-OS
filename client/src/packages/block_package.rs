use super::*;
use crate::bindable::BlockColor;
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct BlockMeta {
    category: String,
    name: String,
    description: String,
    health_boost: i32,
    attack_boost: i8,
    speed_boost: i8,
    charge_boost: i8,
    colors: Vec<String>,
    flat: bool,
    shape: Vec<Vec<u8>>,
}

#[derive(Default, Clone)]
pub struct BlockPackage {
    pub package_info: PackageInfo,
    pub name: String,
    pub description: String,
    pub health_boost: i32,
    pub attack_boost: i8,
    pub speed_boost: i8,
    pub charge_boost: i8,
    pub is_program: bool,
    pub block_colors: Vec<BlockColor>,
    pub shape: [bool; 5 * 5],
}

impl BlockPackage {
    pub fn exists_at(&self, rotation: u8, position: (usize, usize)) -> bool {
        if position.0 >= 5 || position.1 >= 5 {
            return false;
        }

        let (x, y) = match rotation {
            1 => (position.1, 4 - position.0),
            2 => (4 - position.0, 4 - position.1),
            3 => (4 - position.1, position.0),
            _ => (position.0, position.1),
        };

        self.shape.get(y * 5 + x) == Some(&true)
    }
}

impl Package for BlockPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = BlockPackage {
            package_info,
            ..Default::default()
        };

        let meta: BlockMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "block" {
            log::error!(
                "missing `category = \"block\"` in {:?}",
                package.package_info.toml_path
            );
        }

        package.name = meta.name;
        package.description = meta.description;
        package.health_boost = meta.health_boost;
        package.attack_boost = meta.attack_boost;
        package.speed_boost = meta.speed_boost;
        package.charge_boost = meta.charge_boost;
        package.is_program = meta.flat;
        package.block_colors = meta.colors.into_iter().map(BlockColor::from).collect();

        let flattened_shape: Vec<_> = meta.shape.into_iter().flatten().collect();

        if flattened_shape.len() != package.shape.len() {
            log::error!(
                "expected a 5x5 shape (5 lists of 5 numbers) in {:?}",
                package.package_info.toml_path
            );
        }

        for (i, n) in flattened_shape.into_iter().enumerate() {
            package.shape[i] = n != 0;
        }

        package
    }
}
