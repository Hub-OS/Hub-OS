use crate::args::Args;
use crate::overlays::{DebugOverlay, VirtualController};
use crate::packages::*;
use crate::render::{PostProcessAdjust, PostProcessColorBlindness, PostProcessGhosting};
use crate::resources::*;
use crate::saves::GlobalSave;
use crate::supporting_service::SupportingService;
use framework::logging::LogRecord;
use framework::prelude::*;
use walkdir::WalkDir;

enum Event {
    Override {
        resource_path: String,
        file_path: String,
    },
    Complete {
        global_save: Box<GlobalSave>,
        resource_packages: PackageManager<ResourcePackage>,
    },
}

/// Black screen. Loads resources, adds Globals to resources, then advances boot stage
/// We split loading over multiple frames to avoid ANRs on Android
pub struct BootStage1 {
    event_receiver: flume::Receiver<Event>,
    log_receiver: flume::Receiver<LogRecord>,
    global_params: Option<(Args, LocalAssetManager)>,
    next_scene: NextScene,
}

impl BootStage1 {
    pub fn new(game_io: &mut GameIO, args: Args, log_receiver: flume::Receiver<LogRecord>) -> Self {
        let (event_sender, event_receiver) = flume::unbounded();

        BootStage1Thread::spawn(game_io, event_sender);

        Self {
            event_receiver,
            log_receiver,
            global_params: Some((args, LocalAssetManager::new(game_io))),
            next_scene: NextScene::None,
        }
    }

    fn build_globals(
        &mut self,
        game_io: &mut GameIO,
        global_save: GlobalSave,
        resource_packages: PackageManager<ResourcePackage>,
    ) {
        let (args, assets) = self.global_params.take().unwrap();
        let globals = Globals::new(game_io, args, assets, global_save, resource_packages);
        game_io.set_resource(globals);

        let service = SupportingService::new(game_io);
        game_io.add_service(service);

        let ghosting = PostProcessGhosting::new(game_io);
        let adjustments = PostProcessAdjust::new(game_io);
        let color_blindness = PostProcessColorBlindness::new(game_io);
        game_io.add_post_process(ghosting);
        game_io.add_post_process(adjustments);
        game_io.add_post_process(color_blindness);

        let debug_overlay = DebugOverlay::new(game_io);
        game_io.add_overlay(GameOverlayTarget::Render, debug_overlay);

        if cfg!(target_os = "android") {
            let virtual_controller = VirtualController::new(game_io);
            game_io.add_overlay(GameOverlayTarget::Window, virtual_controller);
        }
    }
}

impl Scene for BootStage1 {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        while game_io.frame_start_instant().elapsed() < game_io.target_duration() {
            let Ok(event) = self.event_receiver.try_recv() else {
                break;
            };

            match event {
                Event::Override {
                    resource_path,
                    file_path,
                } => {
                    if let Some((_, assets)) = &mut self.global_params {
                        assets.override_cache(game_io, &resource_path, &file_path);
                    }
                }
                Event::Complete {
                    global_save,
                    resource_packages,
                } => {
                    self.build_globals(game_io, *global_save, resource_packages);

                    let scene =
                        super::boot_stage_2::BootStage2::new(game_io, self.log_receiver.clone());
                    self.next_scene = NextScene::new_push(scene);
                    break;
                }
            }
        }
    }

    fn draw(&mut self, _game_io: &mut GameIO, _render_pass: &mut RenderPass) {}
}

struct BootStage1Thread {
    event_sender: flume::Sender<Event>,
}

impl BootStage1Thread {
    fn spawn(game_io: &mut GameIO, event_sender: flume::Sender<Event>) {
        let assets = LocalAssetManager::new(game_io);

        let mut thread = BootStage1Thread { event_sender };

        std::thread::spawn(move || {
            let _ = thread.load_resources(assets);
        });
    }

    fn load_resources(&mut self, assets: LocalAssetManager) -> anyhow::Result<()> {
        // load save
        let mut global_save = GlobalSave::load(&assets);

        // load resource packages
        let mut resource_packages: PackageManager<ResourcePackage> =
            PackageManager::new(PackageCategory::Resource);

        let _ = resource_packages.load_packages_in_folder(
            &assets,
            PackageNamespace::BuiltIn,
            &ResourcePaths::game_folder_absolute(resource_packages.category().built_in_path()),
            |_, _| -> Result<(), ()> { Ok(()) },
        );

        let _ = resource_packages.load_packages_in_folder(
            &assets,
            PackageNamespace::Local,
            &ResourcePaths::data_folder_absolute(resource_packages.category().mod_path()),
            |_, _| -> Result<(), ()> { Ok(()) },
        );

        // update save to include the new resources
        if global_save.include_new_resources(&resource_packages) {
            global_save.save();
        }

        // apply the final order
        let saved_order = &mut global_save.resource_package_order;

        for (id, enabled) in saved_order.iter() {
            if !enabled {
                continue;
            }

            let Some(package) = resource_packages.package_or_fallback(PackageNamespace::Local, id)
            else {
                continue;
            };

            self.load_package_resources(package)?;
        }

        self.event_sender.send(Event::Complete {
            global_save: Box::new(global_save),
            resource_packages,
        })?;

        Ok(())
    }

    fn load_package_resources(&self, package: &ResourcePackage) -> anyhow::Result<()> {
        let base_path = &package.package_info.base_path;
        let resources_path = base_path.clone() + "resources";
        let path_skip = resources_path.len() - "resources".len();

        for entry in WalkDir::new(resources_path).into_iter().flatten() {
            let Ok(metadata) = entry.metadata() else {
                continue;
            };

            if metadata.is_dir() {
                continue;
            }

            let file_path = &entry.path().to_string_lossy()[..];
            let file_path = file_path.replace(std::path::MAIN_SEPARATOR, ResourcePaths::SEPARATOR);
            let resource_path = file_path[path_skip..].to_string();

            self.event_sender.send(Event::Override {
                resource_path,
                file_path,
            })?;
        }

        Ok(())
    }
}
