use crate::bindable::SpriteColorMode;
use crate::packages::{PackageId, PackageNamespace, PlayerPackage};
use crate::render::ui::{
    ElementSprite, FontName, PlayerHealthUi, SceneTitle, ScrollTracker, SubSceneFrame, TextStyle,
    Textbox, TextboxMessage, UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::*;
use crate::saves::GlobalSave;
use framework::prelude::*;
use itertools::Itertools;
use std::sync::Arc;

const ICON_WIDTH: f32 = 20.0;
const ICON_HEIGHT: f32 = 24.0;
const ICON_MARGIN: f32 = 1.0;
const ICON_X_OFFSET: f32 = ICON_WIDTH + ICON_MARGIN * 2.0;
const ICON_Y_OFFSET: f32 = ICON_HEIGHT + ICON_MARGIN * 2.0;
const SCROLL_STEP: f32 = 0.2;

pub struct CharacterSelectScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    preview_sprite: Sprite,
    health_ui: PlayerHealthUi,
    element_sprite: Sprite,
    name_position: Vec2,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    invalid_sprite: Sprite,
    icon_rows: Vec<IconRow>,
    icon_start_offset: Vec2,
    scroll_offset: Vec2,
    v_scroll_tracker: ScrollTracker,
    h_scroll_tracker: ScrollTracker,
    ui_input_tracker: UiInputTracker,
    textbox: Textbox,
    next_scene: NextScene,
}

impl CharacterSelectScene {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let character_id = &globals.global_save.selected_character;
        let player_package = Self::get_player_package(game_io, character_id);

        let mut package_ids: Vec<_> = globals
            .player_packages
            .package_ids(PackageNamespace::Local)
            .collect();

        package_ids.sort_by_cached_key(|id| Self::get_player_package(game_io, id).name.clone());

        // layout
        let mut ui_animator =
            Animator::load_new(assets, ResourcePaths::CHARACTER_SELECT_UI_ANIMATION);
        ui_animator.set_state("DEFAULT");

        // health_ui
        let mut health_ui = PlayerHealthUi::new(game_io);
        health_ui.set_position(ui_animator.point_or_zero("HP"));
        health_ui.snap_health(player_package.health);

        // element_sprite
        let mut element_sprite = ElementSprite::new(game_io, player_package.element);
        element_sprite.set_position(ui_animator.point_or_zero("ELEMENT"));

        // name_position
        let name_position = ui_animator.point_or_zero("NAME");

        // invalid sprite
        let mut invalid_sprite = assets.new_sprite(game_io, ResourcePaths::CHARACTER_SELECT_UI);
        let invalid_animator = ui_animator.clone().with_state("INVALID_BADGE");

        invalid_animator.apply(&mut invalid_sprite);

        // list bounds
        let list_bounds = Rect::from_corners(
            ui_animator.point_or_zero("LIST_START"),
            ui_animator.point_or_zero("LIST_END"),
        );

        let icons_per_row = ((list_bounds.width + ICON_MARGIN * 2.0) / ICON_X_OFFSET) as usize;
        let row_view_size = ((list_bounds.height + ICON_MARGIN * 2.0) / ICON_Y_OFFSET) as usize;

        // selection
        let selected_index = package_ids
            .iter()
            .position(|id| *id == character_id)
            .unwrap_or_default();
        let v_index = selected_index / icons_per_row;
        let h_index = selected_index % icons_per_row;

        // icons
        let icon_rows: Vec<_> = package_ids
            .into_iter()
            .chunks(icons_per_row)
            .into_iter()
            .map(|package_ids| IconRow::new(game_io, package_ids))
            .collect();

        let row_count = icon_rows.len();
        let col_count = icon_rows
            .get(v_index)
            .map(|row| row.package_count())
            .unwrap_or_default();

        // preview_sprite
        let preview_sprite = Self::load_character_sprite(game_io, player_package);

        // cursor_sprite
        let mut cursor_sprite = assets.new_sprite(game_io, ResourcePaths::CHARACTER_SELECT_CURSOR);
        let cursor_animator =
            Animator::load_new(assets, ResourcePaths::CHARACTER_SELECT_CURSOR_ANIMATION)
                .with_state("DEFAULT")
                .with_loop_mode(AnimatorLoopMode::Loop);

        cursor_animator.apply(&mut cursor_sprite);

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_character_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_everything(true),
            preview_sprite,
            health_ui,
            element_sprite,
            name_position,
            cursor_sprite,
            cursor_animator,
            invalid_sprite,
            icon_start_offset: list_bounds.top_left(),
            icon_rows,
            scroll_offset: Vec2::ZERO,
            v_scroll_tracker: ScrollTracker::new(game_io, row_view_size)
                .with_total_items(row_count)
                .with_selected_index(v_index),
            h_scroll_tracker: ScrollTracker::new(game_io, icons_per_row)
                .with_total_items(col_count)
                .with_wrap(true)
                .with_selected_index(h_index),
            ui_input_tracker: UiInputTracker::new(),
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
        }
    }

    fn get_player_package<'a>(game_io: &'a GameIO, character_id: &PackageId) -> &'a PlayerPackage {
        let globals = game_io.resource::<Globals>().unwrap();

        globals
            .player_packages
            .package(PackageNamespace::Local, character_id)
            .unwrap()
    }

    fn load_character_sprite(game_io: &GameIO, player_package: &PlayerPackage) -> Sprite {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, &player_package.preview_texture_path);
        sprite.set_position(Vec2::new(0.0, RESOLUTION_F.y - sprite.bounds().height));

        sprite
    }

    fn selected_package_id(&self) -> &PackageId {
        let v_index = self.v_scroll_tracker.selected_index();
        let h_index = self.h_scroll_tracker.selected_index();

        self.icon_rows[v_index].get_package_id(h_index)
    }

    fn selected_character_name(&self) -> &str {
        let v_index = self.v_scroll_tracker.selected_index();
        let h_index = self.h_scroll_tracker.selected_index();

        self.icon_rows[v_index].get_character_name(h_index)
    }

    fn calculate_target_scroll(index: usize) -> f32 {
        index as f32 * -ICON_Y_OFFSET
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        // update selection
        let input_tracker = &self.ui_input_tracker;
        let prev_v_index = self.v_scroll_tracker.selected_index();
        let prev_h_index = self.h_scroll_tracker.selected_index();

        self.v_scroll_tracker.handle_vertical_input(input_tracker);
        self.h_scroll_tracker.handle_horizontal_input(input_tracker);

        let v_index = self.v_scroll_tracker.selected_index();
        let h_index = self.h_scroll_tracker.selected_index();

        let v_selection_changed = prev_v_index != v_index;
        let h_selection_changed = prev_h_index != h_index;

        // update total items for the h_scroll_tracker
        if v_selection_changed {
            let count = self.icon_rows[v_index].package_count();
            self.h_scroll_tracker.set_total_items(count);
        }

        // update sprite and play sfx
        if v_selection_changed || h_selection_changed {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);

            let package_id = self.selected_package_id();
            let player_package = Self::get_player_package(game_io, package_id);
            self.preview_sprite = Self::load_character_sprite(game_io, player_package);
            self.health_ui.set_health(player_package.health);

            let old_element_position = self.element_sprite.position();
            self.element_sprite = ElementSprite::new(game_io, player_package.element);
            self.element_sprite.set_position(old_element_position);
        }

        // update cursor
        self.cursor_animator.update();
        self.cursor_animator.apply(&mut self.cursor_sprite);

        // test description
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Option) {
            let globals = game_io.resource::<Globals>().unwrap();

            let package_id = self.selected_package_id();
            let player_packages = &globals.player_packages;
            let package = player_packages
                .package(PackageNamespace::Local, package_id)
                .unwrap();

            // set avatar
            self.textbox
                .set_next_avatar(game_io, &globals.assets, Some(&package.mugshot_paths));

            // set description
            let interface = TextboxMessage::new(package.description.to_string());
            self.textbox.push_interface(interface);
            self.textbox.open();
        }

        // test select
        if input_util.was_just_pressed(Input::Confirm) {
            let globals = game_io.resource_mut::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.v_scroll_tracker.remember_index();
            self.h_scroll_tracker.remember_index();

            let package_id = self.selected_package_id();
            globals.global_save.selected_character = package_id.clone();
            globals.global_save.selected_character_time = GlobalSave::current_time();
            globals.global_save.save();
        }

        // test leave
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            let transition = crate::transitions::new_sub_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }
}

impl Scene for CharacterSelectScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();

        // update health ui
        self.health_ui.update();

        // update textbox
        self.textbox.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close()
        }

        // update input
        self.ui_input_tracker.update(game_io);

        if !game_io.is_in_transition() && !self.textbox.is_open() {
            self.handle_input(game_io);
        }

        // update scroll
        let target_y = Self::calculate_target_scroll(self.v_scroll_tracker.top_index());
        self.scroll_offset.y += (target_y - self.scroll_offset.y) * SCROLL_STEP;
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw character
        sprite_queue.draw_sprite(&self.preview_sprite);

        // draw rows
        let globals = game_io.resource::<Globals>().unwrap();
        let saved_package_id = &globals.global_save.selected_character;
        let v_index = self.v_scroll_tracker.selected_index();
        let h_index = self.h_scroll_tracker.selected_index();

        let top_index = (-self.scroll_offset.y / ICON_Y_OFFSET) as usize;

        let mut offset = self.icon_start_offset + self.scroll_offset;
        offset.y += top_index as f32 * ICON_Y_OFFSET;

        if top_index > 0 {
            offset.y -= ICON_Y_OFFSET;
        }

        let page_end = top_index + self.v_scroll_tracker.view_size() + 2;

        let range_start = top_index.max(1) - 1;
        let range_end = self.icon_rows.len().min(page_end);

        for i in range_start..range_end {
            let row = &mut self.icon_rows[i];

            row.draw(
                game_io,
                &mut sprite_queue,
                &mut self.invalid_sprite,
                offset,
                saved_package_id,
            );

            offset.y += ICON_Y_OFFSET;
        }

        // draw cursor
        let mut offset = self.icon_start_offset + self.scroll_offset;
        offset.x += ICON_X_OFFSET * h_index as f32;
        offset.y += ICON_Y_OFFSET * v_index as f32;

        self.cursor_sprite.set_position(offset);

        sprite_queue.draw_sprite(&self.cursor_sprite);

        // draw name
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(self.name_position);

        let name = self.selected_character_name();
        text_style.draw(game_io, &mut sprite_queue, name);

        // draw overlays
        self.health_ui.draw(game_io, &mut sprite_queue);
        sprite_queue.draw_sprite(&self.element_sprite);

        self.frame.draw(&mut sprite_queue);

        // draw title
        SceneTitle::new("CHARACTER SELECT").draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

struct CompactPackageInfo {
    package_id: PackageId,
    name: Arc<str>,
    texture_path: String,
    animation_path: String,
    valid: bool,
}

struct IconRow {
    compact_package_data: Vec<CompactPackageInfo>,
    sprites: Vec<Sprite>,
}

impl IconRow {
    fn new<'a>(game_io: &GameIO, package_ids: impl Iterator<Item = &'a PackageId>) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let restrictions = &globals.restrictions;
        let player_packages = &globals.player_packages;

        let compact_package_data = package_ids
            .flat_map(|id| player_packages.package(PackageNamespace::Local, id))
            .map(|package| {
                let package_id = &package.package_info.id;
                let valid = restrictions.owns_player(package_id)
                    && restrictions.validate_package_tree(game_io, package.package_info.triplet());

                CompactPackageInfo {
                    package_id: package_id.clone(),
                    name: package.name.clone(),
                    texture_path: package.mugshot_paths.texture.to_string(),
                    animation_path: package.mugshot_paths.animation.to_string(),
                    valid,
                }
            })
            .collect();

        Self {
            compact_package_data,
            sprites: Vec::new(),
        }
    }

    fn get_package_id(&self, index: usize) -> &PackageId {
        &self.compact_package_data[index].package_id
    }

    fn get_character_name(&self, index: usize) -> &str {
        &self.compact_package_data[index].name
    }

    fn package_count(&self) -> usize {
        self.compact_package_data.len()
    }

    fn ensure_sprites(&mut self, game_io: &GameIO) {
        if !self.sprites.is_empty() {
            return;
        }

        self.sprites = (self.compact_package_data)
            .iter()
            .map(|info| Self::load_icon(game_io, info))
            .collect();
    }

    fn load_icon(game_io: &GameIO, info: &CompactPackageInfo) -> Sprite {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, &info.texture_path);
        let mut animator = Animator::load_new(assets, &info.animation_path);

        animator.set_state("IDLE");
        animator.apply(&mut sprite);
        sprite.set_scale(Vec2::new(0.5, 0.5));

        sprite
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        invalid_sprite: &mut Sprite,
        mut offset: Vec2,
        saved_package_id: &PackageId,
    ) {
        self.ensure_sprites(game_io);

        for (i, sprite) in self.sprites.iter_mut().enumerate() {
            let data = &self.compact_package_data[i];

            if data.package_id == *saved_package_id {
                sprite.set_color(Color::WHITE.multiply_color(0.6));
            } else {
                sprite.set_color(Color::WHITE);
            }

            sprite.set_position(offset);
            sprite_queue.draw_sprite(sprite);

            if !data.valid {
                invalid_sprite.set_position(offset);
                sprite_queue.draw_sprite(invalid_sprite);
            }

            offset.x += ICON_X_OFFSET;
        }
    }
}
