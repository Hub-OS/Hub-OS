use crate::packages::{PackageId, PackageNamespace};

pub struct RollbackVM {
    pub package_id: PackageId,
    pub match_namespace: PackageNamespace,
    pub namespaces: Vec<PackageNamespace>,
    pub path: String,
    pub lua: rollback_mlua::Lua,
    pub permitted_dependencies: Vec<PackageId>,
}

impl RollbackVM {
    pub fn preferred_namespace(&self) -> PackageNamespace {
        *self
            .namespaces
            .iter()
            .min_by_key(|ns| match ns {
                PackageNamespace::Server | PackageNamespace::RecordingServer => (0, 0),
                // assuming local namespaces only pass in from encounters
                // battle_props remaps the local namespace to a netplay namespace otherwise
                PackageNamespace::Local | PackageNamespace::RecordingLocal => (1, 0),
                PackageNamespace::BuiltIn | PackageNamespace::RecordingBuiltIn => (2, 0),
                PackageNamespace::Netplay(index) => (3, *index),
            })
            .unwrap()
    }
}
