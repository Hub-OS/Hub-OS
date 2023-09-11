use crate::packages::{PackageId, PackageNamespace};

pub struct RollbackVM {
    pub package_id: PackageId,
    pub namespaces: Vec<PackageNamespace>,
    pub path: String,
    pub lua: rollback_mlua::Lua,
}

impl RollbackVM {
    pub fn preferred_namespace(&self) -> PackageNamespace {
        if self.namespaces.contains(&PackageNamespace::Server) {
            return PackageNamespace::Server;
        }

        if self.namespaces.contains(&PackageNamespace::RecordingServer) {
            return PackageNamespace::RecordingServer;
        }

        let index = self
            .namespaces
            .iter()
            .map(|ns| match ns {
                PackageNamespace::Netplay(index) => *index,
                _ => usize::MAX,
            })
            .min()
            .unwrap();

        PackageNamespace::Netplay(index)
    }
}
