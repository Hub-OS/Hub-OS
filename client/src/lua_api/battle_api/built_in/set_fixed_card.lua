local table, card_properties = ...

local button = table:create_card_button(1)
local player = button:owner()

button:use_card_preview(card_properties)
button:use_fixed_card_cursor(true)

-- create frame
button:set_texture(Resources.game_folder() .. "resources/scenes/battle/card_select.png")
local button_animation = button:animation()
button_animation:load(Resources.game_folder() .. "resources/scenes/battle/card_select.animation")
button_animation:set_state("ICON_FRAME")

-- create icon
player:stage_card(card_properties)
local icon_texture = player:staged_item_texture(#player:staged_items())
player:pop_staged_item()

local button_sprite = button:sprite()
local icon_node = button_sprite:create_node()
icon_node:set_texture(icon_texture)

local code_node = button_sprite:create_text_node("CODE", card_properties.code)
code_node:set_color(Color.new(255, 255, 0))
code_node:set_offset(4, 16)

-- functionality
local used = false

local function card_allowed()
  if card_properties.code == "" then
    return true
  end

  local restriction = player:card_select_restriction()

  if not restriction.code and not restriction.package_id then
    return true
  end

  return ((restriction.code and restriction.code == card_properties.code) or
    (restriction.package_id and restriction.package_id == card_properties.package_id))
end

button.use_func = function()
  if used or not card_allowed() then
    return false
  end

  icon_node:set_visible(false)
  used = true

  player:stage_card(card_properties, function()
    used = false
    icon_node:set_visible(true)
  end)

  return true
end

button.on_selection_change_func = function()
  if card_allowed() then
    icon_node:set_shader_effect(SpriteShaderEffect.None)
  else
    icon_node:set_shader_effect(SpriteShaderEffect.Grayscale)
  end
end
