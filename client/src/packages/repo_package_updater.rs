use super::PackageNamespace;
use crate::render::ui::PackageListing;
use crate::resources::{Globals, ResourcePaths};
use framework::prelude::{AsyncTask, GameIO};
use packets::address_parsing::uri_encode;
use packets::structures::{PackageCategory, PackageId};

enum Event {
    ReceivedListing(Box<PackageListing>),
    InstallPackage,
    Failed,
    Success,
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum UpdateStatus {
    Idle,
    CheckingForUpdate,
    DownloadingPackage,
    Failed,
    Success,
}

pub struct RepoPackageUpdater {
    status: UpdateStatus,
    task: Option<AsyncTask<Event>>,
    queue: Vec<PackageId>,
    queue_position: usize,
    install_required: Vec<(PackageCategory, PackageId, PackageId)>,
    install_position: usize,
}

impl RepoPackageUpdater {
    pub fn new() -> Self {
        Self {
            status: UpdateStatus::Idle,
            task: None,
            queue: Vec::new(),
            queue_position: 0,
            install_required: Vec::new(),
            install_position: 0,
        }
    }

    pub fn total_updated(&self) -> usize {
        self.install_position
    }

    pub fn status(&self) -> UpdateStatus {
        self.status
    }

    pub fn processed_packages(&self) -> usize {
        self.queue_position
    }

    pub fn begin<I>(&mut self, game_io: &GameIO, into_iter: I)
    where
        I: IntoIterator<Item = PackageId>,
    {
        self.queue = into_iter.into_iter().collect();
        self.queue_position = 0;
        self.install_required.clear();
        self.install_position = 0;

        self.request_latest_listing(game_io);
    }

    pub fn update(&mut self, game_io: &mut GameIO) {
        let task_completed = matches!(&self.task, Some(task) if task.is_finished());

        if !task_completed {
            return;
        }

        let task = self.task.take().unwrap();
        let event = task.join().unwrap();

        match event {
            Event::ReceivedListing(listing) => {
                for (_, id) in listing.dependencies {
                    // add only unique dependencies to avoid recursive chains
                    if !self.queue.contains(&id) {
                        self.queue.push(id);
                    }
                }

                let globals = game_io.resource::<Globals>().unwrap();

                // test for required install
                if let Some(category) = listing.preview_data.category() {
                    let id = &self.queue[self.queue_position];
                    let latest_id = listing.id;

                    let existing_hash = globals
                        .package_info(category, PackageNamespace::Local, id)
                        .map(|package| package.hash);

                    let requires_update = existing_hash != Some(listing.hash);
                    let already_updating =
                        (self.install_required.iter()).any(|(_, _, id)| *id == latest_id);

                    if requires_update && !already_updating {
                        // save package id for install pass
                        let old_id = id.clone();
                        self.install_required.push((category, old_id, latest_id));
                    }
                }

                // get the next package
                self.queue_position += 1;
                self.request_latest_listing(game_io);
            }
            Event::InstallPackage => {
                self.install_package(game_io);
            }
            Event::Failed => {
                self.status = UpdateStatus::Failed;
            }
            Event::Success => {
                self.status = UpdateStatus::Success;
            }
        }
    }

    fn request_latest_listing(&mut self, game_io: &GameIO) {
        let Some(id) = self.queue.get(self.queue_position) else {
            self.download_package(game_io);
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();

        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}/meta");

        let task = game_io.spawn_local_task(async move {
            let Some(value) = crate::http::request_json(&uri).await else {
                log::error!("Failed to download meta file");
                return Event::Failed;
            };

            let listing = PackageListing::from(&value);

            Event::ReceivedListing(Box::new(listing))
        });

        self.task = Some(task);
        self.status = UpdateStatus::CheckingForUpdate;
    }

    fn download_package(&mut self, game_io: &GameIO) {
        let Some(&(category, ref old_id, ref id)) =
            self.install_required.get(self.install_position)
        else {
            self.status = UpdateStatus::Success;
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}");
        let base_path = globals.resolve_package_download_path(category, old_id);

        let task = game_io.spawn_local_task(async move {
            let Some(zip_bytes) = crate::http::request(&uri).await else {
                log::error!("Failed to download zip");
                return Event::Failed;
            };

            let _ = std::fs::remove_dir_all(&base_path);

            crate::zip::extract(&zip_bytes, |path, mut virtual_file| {
                let path = format!("{base_path}{path}");

                if let Some(parent_path) = ResourcePaths::parent(&path) {
                    if let Err(err) = std::fs::create_dir_all(parent_path) {
                        log::error!("Failed to create directory {parent_path:?}: {}", err);
                    }
                }

                let res = std::fs::File::create(&path)
                    .and_then(|mut file| std::io::copy(&mut virtual_file, &mut file));

                if let Err(err) = res {
                    log::error!("Failed to write to {path:?}: {}", err);
                }
            });

            Event::InstallPackage
        });

        self.task = Some(task);
        self.status = UpdateStatus::DownloadingPackage;
    }

    fn install_package(&mut self, game_io: &mut GameIO) {
        let (category, old_id, _) = &self.install_required[self.install_position];
        let category = *category;
        self.install_position += 1;

        // reload package
        let globals = game_io.resource_mut::<Globals>().unwrap();
        let path = globals.resolve_package_download_path(category, old_id);

        globals.unload_package(category, PackageNamespace::Local, old_id);
        globals.load_package(category, PackageNamespace::Local, &path);

        // continue working on queue
        self.request_latest_listing(game_io);
    }
}
