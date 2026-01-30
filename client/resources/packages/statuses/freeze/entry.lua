local GAME_FOLDER = Resources.game_folder()
local TEXTURE = Resources.load_texture(GAME_FOLDER .. "resources/scenes/battle/statuses.png")
local SFX = Resources.load_audio(GAME_FOLDER .. "resources/sfx/freeze.ogg")
local FREEZE_COLOR = Color.new(178, 204, 229, 255)

-- shared animator
local animator = Animation.new()
animator:load(GAME_FOLDER .. "resources/scenes/battle/statuses.animation")

-- mashing helper
local inputs = {}

for _, value in pairs(Input.Pressed) do
  inputs[#inputs + 1] = value
end

---@param entity Entity
local function is_mashing(entity)
  for _, value in ipairs(inputs) do
    if entity:input_has(value) then
      return true
    end
  end

  return false
end

---@param parent Entity
local function spawn_alert(parent)
  local alert_artifact = Alert.new()
  alert_artifact:sprite():set_never_flip(true)

  local movement_offset = parent:movement_offset()
  alert_artifact:set_offset(movement_offset.x, movement_offset.y - parent:height())

  Field.spawn(alert_artifact, parent:current_tile())
end

---@param status Status
function status_init(status)
  if not status:reapplied() then
    Resources.play_audio(SFX)
  end

  local entity = status:owner()
  local entity_sprite = entity:sprite()
  local entity_animation = entity:animation()
  local freeze_sprite = entity:create_node()
  freeze_sprite:set_texture(TEXTURE)
  freeze_sprite:set_offset(0, -entity:height() / 2)
  freeze_sprite:set_layer(-1)

  if entity_animation:has_state("CHARACTER_HIT") then
    if not status:reapplied() then
      entity:cancel_movement()
      entity:cancel_actions()
    end

    entity_animation:set_state("CHARACTER_HIT", { { 1, 1 } })
    entity_animation:on_complete(function()
      entity:set_idle()
    end)
  end

  -- remove flash
  entity:remove_status(Hit.Flash)

  local flinch_immunity = AuxProp.new()
      :require_hit_flags_absent(Hit.Flash)
      :declare_immunity(Hit.Flinch)
  entity:add_aux_prop(flinch_immunity)

  -- if we're hit with flinch + flash, cancel freeze
  local cancel_aux_prop = AuxProp.new()
      :require_hit_flags(Hit.Flinch | Hit.Flash)
      :remove_status(Hit.Freeze)
  entity:add_aux_prop(cancel_aux_prop)

  -- resolve state
  local animator_state

  if entity:height() <= 48.0 then
    animator_state = "FREEZE_SMALL"
  elseif entity:height() <= 75.0 then
    animator_state = "FREEZE_MEDIUM"
  else
    animator_state = "FREEZE_LARGE"
  end

  animator:set_state(animator_state)
  animator:apply(freeze_sprite)

  -- this component updates the status's animation and handles mashing
  local component = entity:create_component(Lifetime.ActiveBattle)
  local time = 0

  component.on_update_func = function()
    animator:set_state(animator_state)
    animator:set_playback(Playback.Loop)
    animator:sync_time(time)
    animator:apply(freeze_sprite)
    time = time + 1

    if is_mashing(entity) then
      local remaining_time = status:remaining_time()
      status:set_remaining_time(remaining_time - 1)
    end
  end

  local color_component = entity:create_component(Lifetime.Scene)
  color_component.on_update_func = function()
    freeze_sprite:set_offset(0, -entity:height() / 2)
    entity_sprite:set_color_mode(ColorMode.Add)

    local color = entity:color()
    color.r = FREEZE_COLOR.r
    color.g = FREEZE_COLOR.g
    color.b = FREEZE_COLOR.b
    entity_sprite:set_color(color)
  end

  -- aux props for weaknesses
  local aux_prop_callback = function()
    if status:remaining_time() > 0 then
      status:set_remaining_time(0)
      spawn_alert(entity)
    end
  end

  local break_aux_prop =
      AuxProp.new()
      :require_hit_element(Element.Break)
      :increase_hit_damage("DAMAGE")
      :with_callback(aux_prop_callback)

  local pierce_aux_prop =
      AuxProp.new()
      :require_hit_flags(Hit.PierceGuard)
      :increase_hit_damage("DAMAGE")
      :with_callback(aux_prop_callback)

  entity:add_aux_prop(break_aux_prop)
  entity:add_aux_prop(pierce_aux_prop)

  -- clean up
  status.on_delete_func = function()
    entity_sprite:remove_node(freeze_sprite)
    component:eject()
    color_component:eject()
    entity:remove_aux_prop(flinch_immunity)
    entity:remove_aux_prop(cancel_aux_prop)
    entity:remove_aux_prop(break_aux_prop)
    entity:remove_aux_prop(pierce_aux_prop)
  end
end
