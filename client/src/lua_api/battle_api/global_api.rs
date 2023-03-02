use crate::bindable::LuaVector;
use crate::lua_api::errors::unexpected_nil;
use crate::render::FrameTime;
use crate::resources::ResourcePaths;
use rollback_mlua::LuaSerdeExt;

pub(super) fn inject_global_api(lua: &rollback_mlua::Lua) -> rollback_mlua::Result<()> {
    let globals = lua.globals();

    globals.set("load", rollback_mlua::Nil)?;
    globals.set("loadfile", rollback_mlua::Nil)?;
    globals.set("dofile", rollback_mlua::Nil)?;

    use crate::bindable::HitFlag;

    let hit_flags_table = lua.create_table()?;
    hit_flags_table.set("None", HitFlag::NONE)?;
    hit_flags_table.set("RetainIntangible", HitFlag::RETAIN_INTANGIBLE)?;
    hit_flags_table.set("Freeze", HitFlag::FREEZE)?;
    hit_flags_table.set("PierceInvis", HitFlag::PIERCE_INVIS)?;
    hit_flags_table.set("Flinch", HitFlag::FLINCH)?;
    hit_flags_table.set("Shake", HitFlag::SHAKE)?;
    hit_flags_table.set("Paralyze", HitFlag::PARALYZE)?;
    hit_flags_table.set("Stun", HitFlag::PARALYZE)?; // todo: remove deprecated
    hit_flags_table.set("Flash", HitFlag::FLASH)?;
    hit_flags_table.set("PierceGuard", HitFlag::PIERCE_GUARD)?;
    hit_flags_table.set("Impact", HitFlag::IMPACT)?;
    hit_flags_table.set("Drag", HitFlag::DRAG)?;
    hit_flags_table.set("Bubble", HitFlag::BUBBLE)?;
    hit_flags_table.set("NoCounter", HitFlag::NO_COUNTER)?;
    hit_flags_table.set("Root", HitFlag::ROOT)?;
    hit_flags_table.set("Blind", HitFlag::BLIND)?;
    hit_flags_table.set("Confuse", HitFlag::CONFUSE)?;
    hit_flags_table.set("PierceGround", HitFlag::PIERCE_GROUND)?;
    globals.set("Hit", hit_flags_table)?;

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
    globals.set("Element", element_table)?;

    use crate::bindable::Drag;

    let drag_table = lua.create_table()?;
    drag_table.set(
        "new",
        lua.create_function(|_, (direction, count): (Option<Direction>, Option<u32>)| {
            Ok(Drag {
                direction: direction.unwrap_or_default(),
                count: count.unwrap_or_default(),
            })
        })?,
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
        "unit_vector",
        lua.create_function(|_, direction: Direction| {
            let tuple = direction.unit_vector();

            Ok(LuaVector::from(tuple))
        })?,
    )?;

    globals.set("Direction", direction_table)?;

    let move_event_table = lua.create_table()?;
    move_event_table.set("new", lua.create_function(|lua, _: ()| lua.create_table())?)?;
    globals.set("MoveAction", move_event_table)?;

    use crate::bindable::{EntityId, HitContext, HitFlags, HitProperties};

    let hit_props_table = lua.create_table()?;
    hit_props_table.set(
        "new",
        lua.create_function(|lua, params| {
            let (damage, flags, element, mut rest): (
                i32,
                HitFlags,
                Element,
                rollback_mlua::MultiValue,
            ) = lua.unpack_multi(params)?;

            let middle_param = rest.pop_front().ok_or_else(|| unexpected_nil("Element"))?;

            let secondary_element: Element;
            let context: Option<HitContext>;
            let drag: Drag;

            match lua.unpack(middle_param.clone()) {
                Ok(element) => {
                    secondary_element = element;

                    (context, drag) = lua.unpack_multi(rest)?;
                }
                Err(_) => {
                    secondary_element = Element::None;
                    context = lua.unpack(middle_param)?;
                    drag = lua.unpack_multi(rest)?;
                }
            };

            Ok(HitProperties {
                damage,
                flags,
                element,
                secondary_element,
                aggressor: EntityId::default(),
                drag,
                context: context.unwrap_or_default(),
            })
        })?,
    )?;
    globals.set("HitProps", hit_props_table)?;

    use crate::bindable::CardClass;

    let card_class_table = lua.create_table()?;
    card_class_table.set("Standard", CardClass::Standard)?;
    card_class_table.set("Mega", CardClass::Mega)?;
    card_class_table.set("Giga", CardClass::Giga)?;
    card_class_table.set("Dark", CardClass::Dark)?;
    globals.set("CardClass", card_class_table)?;

    use crate::bindable::BlockColor;

    let block_color_table = lua.create_table()?;
    block_color_table.set("White", BlockColor::White)?;
    block_color_table.set("Red", BlockColor::Red)?;
    block_color_table.set("Green", BlockColor::Green)?;
    block_color_table.set("Blue", BlockColor::Blue)?;
    block_color_table.set("Pink", BlockColor::Pink)?;
    block_color_table.set("Yellow", BlockColor::Yellow)?;
    globals.set("Blocks", block_color_table)?;

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

    use crate::bindable::SpriteColorMode;

    let color_mode_table = lua.create_table()?;
    color_mode_table.set("Multiply", SpriteColorMode::Multiply)?;
    color_mode_table.set("Additive", SpriteColorMode::Add)?;
    globals.set("ColorMode", color_mode_table)?;

    use crate::bindable::AnimatorPlaybackMode;

    let color_mode_table = lua.create_table()?;
    color_mode_table.set("Once", AnimatorPlaybackMode::Once)?;
    color_mode_table.set("Loop", AnimatorPlaybackMode::Loop)?;
    color_mode_table.set("Bounce", AnimatorPlaybackMode::Bounce)?;
    color_mode_table.set("Reverse", AnimatorPlaybackMode::Reverse)?;
    globals.set("Playback", color_mode_table)?;

    use crate::bindable::TileState;

    let tile_state_table = lua.create_table()?;
    tile_state_table.set("Normal", TileState::Normal)?;
    tile_state_table.set("Cracked", TileState::Cracked)?;
    tile_state_table.set("Broken", TileState::Broken)?;
    tile_state_table.set("Ice", TileState::Ice)?;
    tile_state_table.set("Grass", TileState::Grass)?;
    tile_state_table.set("Lava", TileState::Lava)?;
    tile_state_table.set("Poison", TileState::Poison)?;
    tile_state_table.set("Empty", TileState::Empty)?;
    tile_state_table.set("Holy", TileState::Holy)?;
    tile_state_table.set("DirectionLeft", TileState::DirectionLeft)?;
    tile_state_table.set("DirectionRight", TileState::DirectionRight)?;
    tile_state_table.set("DirectionUp", TileState::DirectionUp)?;
    tile_state_table.set("DirectionDown", TileState::DirectionDown)?;
    tile_state_table.set("Volcano", TileState::Volcano)?;
    tile_state_table.set("Sea", TileState::Sea)?;
    tile_state_table.set("Sand", TileState::Sand)?;
    tile_state_table.set("Metal", TileState::Metal)?;
    tile_state_table.set("Hidden", TileState::Hidden)?;
    globals.set("TileState", tile_state_table)?;

    use crate::bindable::TileHighlight;

    let tile_state_table = lua.create_table()?;
    tile_state_table.set("None", TileHighlight::None)?;
    tile_state_table.set("Flash", TileHighlight::Flash)?;
    tile_state_table.set("Solid", TileHighlight::Solid)?;
    globals.set("Highlight", tile_state_table)?;

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

    let lifetimes_table = lua.create_table()?;
    lifetimes_table.set("Local", ComponentLifetime::Local)?;
    lifetimes_table.set("Battle", ComponentLifetime::BattleStep)?;
    lifetimes_table.set("Battlestep", ComponentLifetime::BattleStep)?;
    lifetimes_table.set("Scene", ComponentLifetime::Scene)?;
    globals.set("Lifetimes", lifetimes_table)?;

    use crate::bindable::DefensePriority;

    let defense_priority_table = lua.create_table()?;
    // defense_priority_table.set("Internal", DefensePriority::Internal)?; // internal use only
    // defense_priority_table.set("Intangible", DefensePriority::Intangible)?; // excluded as modders should use set_intangible
    defense_priority_table.set("Barrier", DefensePriority::Barrier)?;
    defense_priority_table.set("Body", DefensePriority::Body)?;
    defense_priority_table.set("CardAction", DefensePriority::CardAction)?;
    defense_priority_table.set("Trap", DefensePriority::Trap)?;
    defense_priority_table.set("Last", DefensePriority::Last)?;
    globals.set("DefensePriority", defense_priority_table)?;

    let defense_order_table = lua.create_table()?;
    defense_order_table.set("Always", false)?;
    defense_order_table.set("CollisionOnly", true)?;
    globals.set("DefenseOrder", defense_order_table)?;

    use crate::bindable::IntangibleRule;

    let intangible_rule_table = lua.create_table()?;
    intangible_rule_table.set(
        "new",
        lua.create_function(|_, ()| Ok(IntangibleRule::default()))?,
    )?;
    globals.set("IntangibleRule", intangible_rule_table)?;

    // todo: ActionOrder, currently stubbed
    globals.set("ActionOrder", lua.create_table()?)?;

    use crate::bindable::ActionLockout;

    globals.set(
        "make_animation_lockout",
        lua.create_function(|lua, _: ()| lua.to_value(&ActionLockout::Animation))?,
    )?;
    globals.set(
        "make_sequence_lockout",
        lua.create_function(|lua, _: ()| lua.to_value(&ActionLockout::Sequence))?,
    )?;
    globals.set(
        "make_async_lockout",
        lua.create_function(|lua, duration: FrameTime| {
            lua.to_value(&ActionLockout::Async(duration))
        })?,
    )?;

    // todo: AudioPriority, currently stubbed
    globals.set("AudioPriority", lua.create_table()?)?;

    let shadow_table = lua.create_table()?;
    shadow_table.set("None", ResourcePaths::BLANK)?;
    shadow_table.set(
        "Small",
        ResourcePaths::absolute(ResourcePaths::BATTLE_SHADOW_SMALL),
    )?;
    shadow_table.set(
        "Big",
        ResourcePaths::absolute(ResourcePaths::BATTLE_SHADOW_BIG),
    )?;
    globals.set("Shadow", shadow_table)?;

    use crate::bindable::InputQuery;
    use crate::resources::Input;

    let input_table = lua.create_table()?;

    let held_table = lua.create_table()?;
    held_table.set("Up", InputQuery::Held(Input::Up))?;
    held_table.set("Left", InputQuery::Held(Input::Left))?;
    held_table.set("Right", InputQuery::Held(Input::Right))?;
    held_table.set("Down", InputQuery::Held(Input::Down))?;
    held_table.set("Use", InputQuery::Held(Input::UseCard))?;
    held_table.set("Special", InputQuery::Held(Input::Special))?;
    held_table.set("Shoot", InputQuery::Held(Input::Shoot))?;
    held_table.set("FaceLeft", InputQuery::Held(Input::FaceLeft))?;
    held_table.set("FaceRight", InputQuery::Held(Input::FaceRight))?;
    held_table.set("LeftShoulder", InputQuery::Held(Input::ShoulderL))?;
    held_table.set("RightShoulder", InputQuery::Held(Input::ShoulderR))?;
    held_table.set("EndTurn", InputQuery::Held(Input::EndTurn))?;
    held_table.set("Ready", InputQuery::Held(Input::End))?;
    input_table.set("Held", held_table)?;

    let pressed_table = lua.create_table()?;
    pressed_table.set("Up", InputQuery::JustPressed(Input::Up))?;
    pressed_table.set("Left", InputQuery::JustPressed(Input::Left))?;
    pressed_table.set("Right", InputQuery::JustPressed(Input::Right))?;
    pressed_table.set("Down", InputQuery::JustPressed(Input::Down))?;
    pressed_table.set("Use", InputQuery::JustPressed(Input::UseCard))?;
    pressed_table.set("Special", InputQuery::JustPressed(Input::Special))?;
    pressed_table.set("Shoot", InputQuery::JustPressed(Input::Shoot))?;
    pressed_table.set("FaceLeft", InputQuery::JustPressed(Input::FaceLeft))?;
    pressed_table.set("FaceRight", InputQuery::JustPressed(Input::FaceRight))?;
    pressed_table.set("LeftShoulder", InputQuery::JustPressed(Input::ShoulderL))?;
    pressed_table.set("RightShoulder", InputQuery::JustPressed(Input::ShoulderR))?;
    pressed_table.set("EndTurn", InputQuery::JustPressed(Input::EndTurn))?;
    pressed_table.set("Ready", InputQuery::JustPressed(Input::End))?;
    input_table.set("Pressed", pressed_table)?;

    globals.set("Input", input_table)?;

    Ok(())
}
