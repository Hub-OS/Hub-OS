use crate::packages::PackageNamespace;

pub struct RollbackVM {
    pub package_id: String,
    pub namespace: PackageNamespace,
    pub path: String,
    pub lua: rollback_mlua::Lua,
}
