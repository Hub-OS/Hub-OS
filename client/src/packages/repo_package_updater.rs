use super::PackageNamespace;
use crate::render::ui::PackageListing;
use crate::resources::{Globals, ResourcePaths};
use framework::prelude::{AsyncTask, GameIO};
use packets::address_parsing::uri_encode;
use packets::structures::{PackageCategory, PackageId};

enum Event {
    ReceivedListing(PackageListing),
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
    queue: Vec<(Option<PackageCategory>, PackageId)>,
    queue_position: usize,
    total_updated: usize,
}

impl RepoPackageUpdater {
    pub fn new() -> Self {
        Self {
            status: UpdateStatus::Idle,
            task: None,
            queue: Vec::new(),
            queue_position: 0,
            total_updated: 0,
        }
    }

    pub fn total_updated(&self) -> usize {
        self.total_updated
    }

    pub fn status(&self) -> UpdateStatus {
        self.status
    }

    pub fn processed_packages(&self) -> usize {
        self.queue_position
    }

    pub fn begin<I>(&mut self, game_io: &mut GameIO, i: I)
    where
        I: IntoIterator<Item = PackageId>,
    {
        self.queue.clear();
        self.queue_position = 0;
        self.total_updated = 0;

        self.queue.extend(i.into_iter().map(|id| (None, id)));

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
                for (category, id) in listing.dependencies {
                    let dependency = (Some(category), id);

                    // add only unique dependencies to avoid recursive chains
                    if !self.queue.contains(&dependency) {
                        self.queue.push(dependency)
                    }
                }

                let (optional_category, id) = self.queue.get_mut(self.queue_position).unwrap();
                *optional_category = listing.preview_data.category();

                let globals = game_io.resource::<Globals>().unwrap();
                let existing_hash = if let Some(category) = optional_category {
                    globals
                        .package_or_fallback_info(*category, PackageNamespace::Local, id)
                        .map(|package| package.hash)
                } else {
                    None
                };

                if existing_hash == Some(listing.hash) {
                    // already up to date, get the next package
                    self.queue_position += 1;
                    self.request_latest_listing(game_io);
                } else {
                    self.download_package(game_io)
                }
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

    fn request_latest_listing(&mut self, game_io: &mut GameIO) {
        let Some((_, id)) = self.queue.get(self.queue_position) else {
            self.status = UpdateStatus::Success;
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();

        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}/meta");

        let task = game_io.spawn_local_task(async move {
            let Some(value) = crate::http::request_json(&uri).await else {
                return Event::Failed;
            };

            let listing = PackageListing::from(&value);

            Event::ReceivedListing(listing)
        });

        self.task = Some(task);
        self.status = UpdateStatus::CheckingForUpdate;
    }

    fn download_package(&mut self, game_io: &mut GameIO) {
        let Some((category, id)) = self.queue.get(self.queue_position) else {
            self.status = UpdateStatus::Success;
            return;
        };

        let Some(category) = category.clone() else {
            // can't download this (special category such as "pack")
            // move onto the next queue item
            self.queue_position += 1;
            self.request_latest_listing(game_io);
            return;
        };

        self.total_updated += 1;

        let globals = game_io.resource::<Globals>().unwrap();
        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}");
        let base_path = globals.resolve_package_download_path(category, id);

        let task = game_io.spawn_local_task(async move {
            let Some(zip_bytes) = crate::http::request(&uri).await else {
                return Event::Failed;
            };

            let _ = std::fs::remove_dir_all(&base_path);

            crate::zip::extract(&zip_bytes, |path, mut virtual_file| {
                let path = format!("{base_path}{path}");

                if let Some(parent_path) = ResourcePaths::parent(&path) {
                    if let Err(err) = std::fs::create_dir_all(parent_path) {
                        log::error!("failed to create directory {parent_path:?}: {}", err);
                    }
                }

                let res = std::fs::File::create(&path)
                    .and_then(|mut file| std::io::copy(&mut virtual_file, &mut file));

                if let Err(err) = res {
                    log::error!("failed to write to {path:?}: {}", err);
                }
            });

            Event::InstallPackage
        });

        self.task = Some(task);
        self.status = UpdateStatus::DownloadingPackage;
    }

    fn install_package(&mut self, game_io: &mut GameIO) {
        let (category, id) = self.queue[self.queue_position].clone();
        self.queue_position += 1;

        let Some(category) = category else {
            // special category such as "pack"
            // just continue working on the queue
            self.request_latest_listing(game_io);
            return;
        };

        // reload package
        let globals = game_io.resource_mut::<Globals>().unwrap();
        let path = globals.resolve_package_download_path(category, &id);

        globals.unload_package(category, PackageNamespace::Local, &id);
        globals.load_package(category, PackageNamespace::Local, &path);

        // resolve player package
        let global_save = &mut globals.global_save;
        let selected_player_exists = globals
            .player_packages
            .package(PackageNamespace::Local, &global_save.selected_character)
            .is_some();

        if category == PackageCategory::Player && !selected_player_exists {
            global_save.selected_character = id;
        }

        // continue working on queue
        self.request_latest_listing(game_io);
    }
}
