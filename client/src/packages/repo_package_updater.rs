use super::PackageNamespace;
use crate::render::ui::PackageListing;
use crate::resources::Globals;
use framework::prelude::{AsyncTask, GameIO};
use packets::address_parsing::uri_encode;
use packets::structures::{PackageCategory, PackageId};
use std::collections::{HashMap, HashSet};

struct InstallMeta {
    category: PackageCategory,
    current_id: PackageId,
    new_id: PackageId,
    past_ids: Vec<PackageId>,
}

enum Event {
    ReceivedListing(Box<PackageListing>),
    FailedListing(PackageId),
    InstallPackage,
    FailedDownload,
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum UpdateStatus {
    Idle,
    CheckingForUpdate,
    DownloadingPackage,
    Success,
    Failed,
}

pub struct RepoPackageUpdater {
    status: UpdateStatus,
    task: Option<AsyncTask<Event>>,
    queue: Vec<PackageId>,
    queue_position: usize,
    install_required: Vec<InstallMeta>,
    install_position: usize,
    ignoring_installed_dependencies: bool,
    dependent_map: HashMap<PackageId, Vec<PackageId>>,
    cancelled_installs: HashSet<PackageId>,
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
            ignoring_installed_dependencies: false,
            dependent_map: Default::default(),
            cancelled_installs: Default::default(),
        }
    }

    /// Used when we've already checked the hashes for installed dependencies -> when checking for updates to all packages
    pub fn with_ignore_installed_dependencies(mut self, ignore: bool) -> Self {
        self.ignoring_installed_dependencies = ignore;
        self
    }

    pub fn total_updates(&self) -> usize {
        self.install_required.len()
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

        self.request_listing(game_io);
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
                let globals = Globals::from_resources_mut(game_io);

                for (category, id) in &listing.dependencies {
                    if self.ignoring_installed_dependencies
                        && globals
                            .package_info(*category, PackageNamespace::Local, id)
                            .is_some()
                    {
                        // avoid checking packages that are already installed
                        continue;
                    }

                    // add only unique dependencies to avoid recursive chains
                    if !self.queue.contains(id) {
                        self.queue.push(id.clone());
                    }
                }

                // test for required install
                let latest_id = listing.id;
                let already_updating =
                    (self.install_required.iter()).any(|meta| meta.new_id == latest_id);

                if let Some(category) = listing.preview_data.category()
                    && !already_updating
                {
                    let queued_id = &self.queue[self.queue_position];

                    for (_, id) in listing.dependencies {
                        let dependents = self.dependent_map.entry(id).or_default();
                        dependents.push(latest_id.clone())
                    }

                    let existing_package = globals
                        .package_info(category, PackageNamespace::Local, queued_id)
                        .or_else(|| {
                            globals.package_info(category, PackageNamespace::Local, &latest_id)
                        });

                    let existing_hash = existing_package.map(|package| package.hash);
                    let requires_update = existing_hash != Some(listing.hash);

                    // queue an update update if necessary
                    if requires_update {
                        // prefer installed packages matching the latest id
                        let newer_id_installed = globals
                            .package_info(category, PackageNamespace::Local, &latest_id)
                            .is_some_and(|p| p.id == latest_id);

                        // save package id for install pass
                        let current_id = if newer_id_installed {
                            latest_id.clone()
                        } else {
                            queued_id.clone()
                        };

                        self.install_required.push(InstallMeta {
                            category,
                            current_id,
                            new_id: latest_id,
                            past_ids: listing.past_ids,
                        });
                    }
                }

                self.request_next_listing(game_io);
            }
            Event::FailedListing(package_id) => {
                self.cancelled_installs.insert(package_id);
                self.request_next_listing(game_io);
            }
            Event::InstallPackage => {
                self.install_package(game_io);
            }
            Event::FailedDownload => {
                self.status = UpdateStatus::Failed;
            }
        }
    }

    fn request_next_listing(&mut self, game_io: &GameIO) {
        self.queue_position += 1;
        self.request_listing(game_io);
    }

    fn request_listing(&mut self, game_io: &GameIO) {
        let Some(id) = self.queue.get(self.queue_position) else {
            self.cancel_unsatisfied_installs();
            self.download_package(game_io);
            return;
        };

        let globals = Globals::from_resources(game_io);

        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}/meta");

        log::info!("Requesting metadata for {id}...");

        let id = id.clone();
        let task = game_io.spawn_local_task(async move {
            let Some(value) = crate::http::request_json(&uri).await else {
                log::error!("Failed to download meta file");
                return Event::FailedListing(id);
            };

            let listing = PackageListing::from(&value);

            Event::ReceivedListing(Box::new(listing))
        });

        self.task = Some(task);
        self.status = UpdateStatus::CheckingForUpdate;
    }

    fn cancel_unsatisfied_installs(&mut self) {
        let mut queue: Vec<_> = self.cancelled_installs.iter().cloned().collect();

        while let Some(dependency_id) = queue.pop() {
            let Some(dependents) = self.dependent_map.remove(&dependency_id) else {
                continue;
            };

            for dependent_id in dependents {
                if !self.cancelled_installs.insert(dependent_id.clone()) {
                    continue;
                }

                log::warn!("Skipping {dependent_id} due to unsatisfied dependencies");

                queue.push(dependent_id);
            }
        }

        self.install_required
            .retain(|meta| !self.cancelled_installs.contains(&meta.new_id));
    }

    fn download_package(&mut self, game_io: &GameIO) {
        let Some(meta) = self.install_required.get(self.install_position) else {
            self.complete();
            return;
        };

        let category = meta.category;

        let globals = Globals::from_resources(game_io);
        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(meta.new_id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}");
        let base_path = globals.resolve_package_download_path(category, &meta.current_id);

        log::info!("Downloading {}...", meta.new_id);

        let task = game_io.spawn_local_task(async move {
            let Some(zip_bytes) = crate::http::request(&uri).await else {
                log::error!("Failed to download zip");
                return Event::FailedDownload;
            };

            packets::zip::extract_to(&zip_bytes, &base_path);

            Event::InstallPackage
        });

        self.task = Some(task);
        self.status = UpdateStatus::DownloadingPackage;
    }

    fn install_package(&mut self, game_io: &mut GameIO) {
        const NS: PackageNamespace = PackageNamespace::Local;

        let meta = &self.install_required[self.install_position];
        let category = meta.category;

        // reload package
        let globals = Globals::from_resources_mut(game_io);
        let path = globals.resolve_package_download_path(category, &meta.current_id);

        globals.unload_package(category, NS, &meta.current_id);

        for id in &meta.past_ids {
            let Some(old_package) = globals.package_info(category, NS, id) else {
                continue;
            };

            log::info!(
                "Removing {} as newer package {id} is installed",
                old_package.id
            );

            // uninstall old package
            let base_path = globals.resolve_package_download_path(category, &old_package.id);
            let _ = std::fs::remove_dir_all(base_path);

            globals.unload_package(category, NS, id);
            globals.global_save.remove_package_id(id);
        }

        globals.load_package(category, NS, &path);

        // update save
        if meta.current_id != meta.new_id {
            let global_save = &mut globals.global_save;
            global_save.update_package_id(category, &meta.current_id, &meta.new_id);
            global_save.save();
        }

        // download next package
        self.install_position += 1;
        self.download_package(game_io);
    }

    fn complete(&mut self) {
        self.status = if self.cancelled_installs.is_empty() {
            UpdateStatus::Success
        } else {
            UpdateStatus::Failed
        }
    }
}
