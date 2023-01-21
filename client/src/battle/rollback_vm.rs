use crate::packages::{PackageId, PackageNamespace};

pub struct RollbackVM {
    pub package_id: PackageId,
    pub namespace: PackageNamespace,
    pub path: String,
    pub lua: rollback_mlua::Lua,
}
