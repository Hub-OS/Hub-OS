-- implementation in rust, we only display the blind indicator here
local GAME_FOLDER = Resources.game_folder()
local TEXTURE = Resources.load_texture(GAME_FOLDER .. "resources/scenes/battle/statuses.png")

-- shared animator
local animator = Animation.new()
animator:load(GAME_FOLDER .. "resources/scenes/battle/statuses.animation")
animator:set_state("BLIND")
animator:set_playback(Playback.Loop)

---@param status Status
function status_init(status)
  local entity = status:owner()

  local blind_sprite = entity:create_node()
  blind_sprite:set_texture(TEXTURE)
  blind_sprite:set_offset(0, -entity:height())
  blind_sprite:set_layer(-1)
  animator:apply(blind_sprite)

  -- this component updates the status's animation
  local component = entity:create_component(Lifetime.ActiveBattle)
  local time = 0

  component.on_update_func = function()
    animator:sync_time(time)
    animator:apply(blind_sprite)
    time = time + 1
  end

  status.on_delete_func = function()
    component:eject()
    entity:sprite():remove_node(blind_sprite)
  end
end
