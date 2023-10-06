use crate::packages::{PackageId, PackageNamespace};

pub struct RollbackVM {
    pub package_id: PackageId,
    pub namespaces: Vec<PackageNamespace>,
    pub path: String,
    pub lua: rollback_mlua::Lua,
}

impl RollbackVM {
    pub fn preferred_namespace(&self) -> PackageNamespace {
        let is_server_or_built_in = |namespace: &&PackageNamespace| {
            matches!(
                namespace,
                PackageNamespace::Server
                    | PackageNamespace::RecordingServer
                    | PackageNamespace::BuiltIn
            )
        };

        if let Some(namespace) = self.namespaces.iter().find(is_server_or_built_in) {
            return *namespace;
        }

        let index = self
            .namespaces
            .iter()
            .map(|ns| match ns {
                PackageNamespace::Netplay(index) => *index,
                _ => u8::MAX,
            })
            .min()
            .unwrap();

        PackageNamespace::Netplay(index)
    }
}
