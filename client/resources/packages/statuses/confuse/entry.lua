local GAME_FOLDER = Resources.game_folder()
local TEXTURE = Resources.load_texture(GAME_FOLDER .. "resources/scenes/battle/statuses.png")

-- shared animator
local animator = Animation.new()
animator:load(GAME_FOLDER .. "resources/scenes/battle/statuses.animation")
animator:set_state("CONFUSE")
animator:set_playback(Playback.Loop)

---@param status Status
function status_init(status)
  local entity = status:owner()

  local confuse_sprite = entity:create_node()
  confuse_sprite:set_texture(TEXTURE)
  confuse_sprite:set_offset(0, -entity:height())
  confuse_sprite:set_layer(-1)
  animator:apply(confuse_sprite)

  -- this component updates the status's animation
  local component = entity:create_component(Lifetime.ActiveBattle)
  local time = 0

  component.on_update_func = function()
    animator:sync_time(time)
    animator:apply(confuse_sprite)
    time = time + 1
  end

  if Player.from(entity) then
    entity:boost_augment("dev.hubos.Augments.Confuse", 1)
  end

  status.on_delete_func = function()
    if Player.from(entity) then
      entity:boost_augment("dev.hubos.Augments.Confuse", -1)
    end

    component:eject()
    entity:sprite():remove_node(confuse_sprite)
  end
end
