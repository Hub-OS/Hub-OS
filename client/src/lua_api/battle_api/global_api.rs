use crate::battle::ActionType;
use crate::bindable::{AudioBehavior, LuaVector, TimeFreezeChainLimit};
use crate::lua_api::battle_api::errors::warn_deprecated;
use crate::render::FrameTime;
use crate::resources::ResourcePaths;
use rollback_mlua::LuaSerdeExt;

pub(super) fn inject_global_api(lua: &rollback_mlua::Lua) -> rollback_mlua::Result<()> {
    let globals = lua.globals();

    globals.set("load", rollback_mlua::Nil)?;
    globals.set("loadfile", rollback_mlua::Nil)?;
    globals.set("dofile", rollback_mlua::Nil)?;

    let tfc_chain_limit_table = lua.create_table()?;
    tfc_chain_limit_table.set("OnePerEntity", TimeFreezeChainLimit::OnePerEntity)?;
    tfc_chain_limit_table.set("OnePerTeam", TimeFreezeChainLimit::OnePerTeam)?;
    tfc_chain_limit_table.set("Unlimited", TimeFreezeChainLimit::Unlimited)?;
    globals.set("TimeFreezeChainLimit", tfc_chain_limit_table)?;

    let element_table = lua.create_table()?;
    element_table.set("None", Element::None)?;
    element_table.set("Fire", Element::Fire)?;
    element_table.set("Aqua", Element::Aqua)?;
    element_table.set("Elec", Element::Elec)?;
    element_table.set("Wood", Element::Wood)?;
    element_table.set("Sword", Element::Sword)?;
    element_table.set("Wind", Element::Wind)?;
    element_table.set("Cursor", Element::Cursor)?;
    element_table.set("Summon", Element::Summon)?;
    element_table.set("Plus", Element::Plus)?;
    element_table.set("Break", Element::Break)?;

    element_table.set(
        "is_weak_to",
        lua.create_function(|_, (a, b): (Element, Element)| Ok(a.is_weak_to(b)))?,
    )?;

    globals.set("Element", element_table)?;

    use crate::bindable::Drag;

    let drag_table = lua.create_table()?;
    drag_table.set(
        "new",
        lua.create_function(
            |_, (direction, distance): (Option<Direction>, Option<u32>)| {
                Ok(Drag {
                    direction: direction.unwrap_or_default(),
                    distance: distance.unwrap_or_default(),
                })
            },
        )?,
    )?;
    drag_table.set("None", Drag::default())?;
    globals.set("Drag", drag_table)?;

    use crate::bindable::Direction;

    let direction_table = lua.create_table()?;
    direction_table.set("None", Direction::None)?;
    direction_table.set("Up", Direction::Up)?;
    direction_table.set("Left", Direction::Left)?;
    direction_table.set("Down", Direction::Down)?;
    direction_table.set("Right", Direction::Right)?;
    direction_table.set("UpLeft", Direction::UpLeft)?;
    direction_table.set("UpRight", Direction::UpRight)?;
    direction_table.set("DownLeft", Direction::DownLeft)?;
    direction_table.set("DownRight", Direction::DownRight)?;

    use crate::bindable::Element;
    // util
    direction_table.set(
        "flip_x",
        lua.create_function(|_, direction: Direction| Ok(direction.horizontal_mirror()))?,
    )?;
    direction_table.set(
        "flip_y",
        lua.create_function(|_, direction: Direction| Ok(direction.vertical_mirror()))?,
    )?;
    direction_table.set(
        "reverse",
        lua.create_function(|_, direction: Direction| Ok(direction.reversed()))?,
    )?;
    direction_table.set(
        "join",
        lua.create_function(|_, (a, b): (Direction, Direction)| Ok(a.join(b)))?,
    )?;
    direction_table.set(
        "split",
        lua.create_function(|_, direction: Direction| Ok(direction.split()))?,
    )?;
    direction_table.set(
        "vector",
        lua.create_function(|_, direction: Direction| {
            let tuple = direction.chebyshev_vector();

            Ok(LuaVector::from(tuple))
        })?,
    )?;
    direction_table.set(
        "unit_vector",
        lua.create_function(|_, direction: Direction| {
            let tuple = direction.unit_vector();

            Ok(LuaVector::from(tuple))
        })?,
    )?;

    globals.set("Direction", direction_table)?;

    use crate::bindable::CardClass;

    let card_class_table = lua.create_table()?;
    card_class_table.set("Standard", CardClass::Standard)?;
    card_class_table.set("Mega", CardClass::Mega)?;
    card_class_table.set("Giga", CardClass::Giga)?;
    card_class_table.set("Dark", CardClass::Dark)?;
    card_class_table.set("Recipe", CardClass::Recipe)?;
    globals.set("CardClass", card_class_table)?;

    use crate::bindable::LuaColor;
    use framework::prelude::Color;

    let color_table = lua.create_table()?;
    color_table.set(
        "new",
        lua.create_function(|_lua, (r, g, b, a): (u8, u8, u8, Option<u8>)| {
            Ok(LuaColor::new(r, g, b, a.unwrap_or(255)))
        })?,
    )?;
    color_table.set(
        "mix",
        lua.create_function(|_lua, (a, b, mut percent): (LuaColor, LuaColor, f32)| {
            percent = percent.clamp(0.0, 1.0);

            let color = Color::lerp(a.into(), b.into(), percent);

            Ok(LuaColor::from(color))
        })?,
    )?;
    globals.set("Color", color_table)?;

    use crate::render::SpriteShaderEffect;

    let sprite_shader_table = lua.create_table()?;
    sprite_shader_table.set("None", SpriteShaderEffect::Default)?;
    sprite_shader_table.set("Grayscale", SpriteShaderEffect::Grayscale)?;
    sprite_shader_table.set("Pixelate", SpriteShaderEffect::Pixelate)?;
    globals.set("SpriteShaderEffect", sprite_shader_table)?;

    use crate::bindable::SpriteColorMode;

    let color_mode_table = lua.create_table()?;
    let color_mode_metatable = lua.create_table()?;
    color_mode_metatable.raw_set(
        "__index",
        lua.create_function(
            |lua, (self_table, key): (rollback_mlua::Table, rollback_mlua::String)| {
                let value = self_table.raw_get::<_, rollback_mlua::Value>(key.clone())?;

                if !value.is_nil() {
                    return lua.pack_multi(value);
                }

                if key.to_string_lossy() == "Additive" {
                    warn_deprecated(lua, "ColorMode.Additive (use ColorMode.Add instead)");
                    self_table.set("Additive", SpriteColorMode::Add)?;
                    return lua.pack_multi(SpriteColorMode::Add);
                }

                lua.pack_multi(rollback_mlua::Nil)
            },
        )?,
    )?;
    color_mode_table.set_metatable(Some(color_mode_metatable));

    color_mode_table.set("Multiply", SpriteColorMode::Multiply)?;
    color_mode_table.set("Add", SpriteColorMode::Add)?;
    color_mode_table.set("Adopt", SpriteColorMode::Adopt)?;
    globals.set("ColorMode", color_mode_table)?;

    use crate::bindable::AnimatorPlaybackMode;

    let color_mode_table = lua.create_table()?;
    color_mode_table.set("Once", AnimatorPlaybackMode::Once)?;
    color_mode_table.set("Loop", AnimatorPlaybackMode::Loop)?;
    color_mode_table.set("Bounce", AnimatorPlaybackMode::Bounce)?;
    color_mode_table.set("Reverse", AnimatorPlaybackMode::Reverse)?;
    globals.set("Playback", color_mode_table)?;

    use crate::bindable::TileHighlight;

    let tile_highlight_table = lua.create_table()?;
    tile_highlight_table.set("None", TileHighlight::None)?;
    tile_highlight_table.set("Flash", TileHighlight::Flash)?;
    tile_highlight_table.set("Solid", TileHighlight::Solid)?;
    globals.set("Highlight", tile_highlight_table)?;

    use crate::bindable::Team;

    let team_table = lua.create_table()?;
    team_table.set("Other", Team::Other)?;
    team_table.set("Red", Team::Red)?;
    team_table.set("Blue", Team::Blue)?;
    globals.set("Team", team_table)?;

    use crate::bindable::CharacterRank;

    let rank_table = lua.create_table()?;
    rank_table.set("V1", CharacterRank::V1)?;
    rank_table.set("V2", CharacterRank::V2)?;
    rank_table.set("V3", CharacterRank::V3)?;
    rank_table.set("V4", CharacterRank::V4)?;
    rank_table.set("V5", CharacterRank::V5)?;
    rank_table.set("SP", CharacterRank::SP)?;
    rank_table.set("EX", CharacterRank::EX)?;
    rank_table.set("Rare1", CharacterRank::Rare1)?;
    rank_table.set("Rare2", CharacterRank::Rare2)?;
    rank_table.set("NM", CharacterRank::NM)?;
    rank_table.set("RV", CharacterRank::RV)?;
    rank_table.set("DS", CharacterRank::DS)?;
    rank_table.set("Alpha", CharacterRank::Alpha)?;
    rank_table.set("Beta", CharacterRank::Beta)?;
    rank_table.set("Omega", CharacterRank::Omega)?;
    rank_table.set("Sigma", CharacterRank::Sigma)?;
    globals.set("Rank", rank_table)?;

    use crate::bindable::ComponentLifetime;

    let lifetime_table = lua.create_table()?;
    lifetime_table.set("Local", ComponentLifetime::Local)?;
    lifetime_table.set("ActiveBattle", ComponentLifetime::ActiveBattle)?;
    lifetime_table.set("Battle", ComponentLifetime::Battle)?;
    lifetime_table.set("Scene", ComponentLifetime::Scene)?;
    lifetime_table.set("CardSelectOpen", ComponentLifetime::CardSelectOpen)?;
    lifetime_table.set("CardSelectClose", ComponentLifetime::CardSelectClose)?;
    lifetime_table.set("CardSelectComplete", ComponentLifetime::CardSelectComplete)?;
    lifetime_table.set("Nil", ComponentLifetime::Nil)?;
    globals.set("Lifetime", lifetime_table)?;

    use crate::bindable::DefensePriority;

    let defense_priority_table = lua.create_table()?;
    // defense_priority_table.set("Internal", DefensePriority::Internal)?; // internal use only
    // defense_priority_table.set("Intangible", DefensePriority::Intangible)?; // excluded as modders should use set_intangible
    defense_priority_table.set("Barrier", DefensePriority::Barrier)?;
    defense_priority_table.set("Action", DefensePriority::Action)?;
    defense_priority_table.set("Body", DefensePriority::Body)?;
    defense_priority_table.set("Trap", DefensePriority::Trap)?;
    defense_priority_table.set("Last", DefensePriority::Last)?;
    globals.set("DefensePriority", defense_priority_table)?;

    let defense_order_table = lua.create_table()?;
    defense_order_table.set("Always", false)?;
    defense_order_table.set("CollisionOnly", true)?;
    globals.set("DefenseOrder", defense_order_table)?;

    let action_type = lua.create_table()?;
    action_type.set("All", ActionType::ALL)?;
    action_type.set("Scripted", ActionType::SCRIPT)?;
    action_type.set("Normal", ActionType::NORMAL)?;
    action_type.set("Charged", ActionType::CHARGED)?;
    action_type.set("Special", ActionType::SPECIAL)?;
    action_type.set("Card", ActionType::CARD)?;
    globals.set("ActionType", action_type)?;

    // todo: ActionOrder, currently stubbed
    globals.set("ActionOrder", lua.create_table()?)?;

    use crate::bindable::ActionLockout;

    let action_lockout = lua.create_table()?;
    action_lockout.set(
        "new_animation",
        lua.create_function(|lua, _: ()| lua.to_value(&ActionLockout::Animation))?,
    )?;
    action_lockout.set(
        "new_sequence",
        lua.create_function(|lua, _: ()| lua.to_value(&ActionLockout::Sequence))?,
    )?;
    action_lockout.set(
        "new_async",
        lua.create_function(|lua, duration: FrameTime| {
            lua.to_value(&ActionLockout::Async(duration))
        })?,
    )?;
    globals.set("ActionLockout", action_lockout)?;

    let audio_behavior_table = lua.create_table()?;
    audio_behavior_table.set("Default", AudioBehavior::Default)?;
    audio_behavior_table.set("Restart", AudioBehavior::Restart)?;
    audio_behavior_table.set("Overlap", AudioBehavior::Overlap)?;
    audio_behavior_table.set("NoOverlap", AudioBehavior::NoOverlap)?;
    audio_behavior_table.set(
        "LoopSection",
        lua.create_function(|_, (start, end)| Ok(AudioBehavior::LoopSection(start, end)))?,
    )?;
    audio_behavior_table.set("EndLoop", AudioBehavior::EndLoop)?;
    globals.set("AudioBehavior", audio_behavior_table)?;

    let shadow_table = lua.create_table()?;
    shadow_table.set("None", ResourcePaths::BLANK)?;
    shadow_table.set(
        "Small",
        ResourcePaths::game_folder_absolute(ResourcePaths::BATTLE_SHADOW_SMALL),
    )?;
    shadow_table.set(
        "Big",
        ResourcePaths::game_folder_absolute(ResourcePaths::BATTLE_SHADOW_BIG),
    )?;
    globals.set("Shadow", shadow_table)?;

    use crate::bindable::InputQuery;
    use crate::resources::Input;

    let input_pairs = [
        ("Up", Input::Up),
        ("Left", Input::Left),
        ("Right", Input::Right),
        ("Down", Input::Down),
        ("Use", Input::UseCard),
        ("Special", Input::Special),
        ("Shoot", Input::Shoot),
        ("FaceLeft", Input::FaceLeft),
        ("FaceRight", Input::FaceRight),
        ("LeftShoulder", Input::ShoulderL),
        ("RightShoulder", Input::ShoulderR),
        ("EndTurn", Input::EndTurn),
        ("Ready", Input::End),
        ("Confirm", Input::Confirm),
        ("Cancel", Input::Cancel),
    ];

    let input_table = lua.create_table()?;

    let held_table = lua.create_table()?;
    for (name, input) in input_pairs {
        held_table.set(name, InputQuery::Held(input))?;
    }
    input_table.set("Held", held_table)?;

    let pressed_table = lua.create_table()?;
    for (name, input) in input_pairs {
        pressed_table.set(name, InputQuery::JustPressed(input))?;
    }
    input_table.set("Pressed", pressed_table)?;

    let pulsed_table = lua.create_table()?;
    for (name, input) in input_pairs {
        pulsed_table.set(name, InputQuery::Pulsed(input))?;
    }
    input_table.set("Pulsed", pulsed_table)?;

    globals.set("Input", input_table)?;

    use crate::bindable::Comparison;

    let comparison_table = lua.create_table()?;
    comparison_table.set("LT", Comparison::LT)?;
    comparison_table.set("LE", Comparison::LE)?;
    comparison_table.set("EQ", Comparison::EQ)?;
    comparison_table.set("NE", Comparison::NE)?;
    comparison_table.set("GT", Comparison::GT)?;
    comparison_table.set("GE", Comparison::GE)?;
    globals.set("Compare", comparison_table)?;

    Ok(())
}
