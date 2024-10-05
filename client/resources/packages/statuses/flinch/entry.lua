local GAME_FOLDER = Resources.game_folder()
local SFX = Resources.load_audio(GAME_FOLDER .. "resources/sfx/hurt.ogg")

---@param status Status
function status_init(status)
  local entity = status:owner()
  local animation = entity:animation()

  if not animation:has_state("CHARACTER_HIT") then
    return
  end

  entity:cancel_actions()
  entity:cancel_movement()

  local action = Action.new(entity, "CHARACTER_HIT")
  action:override_animation_frames({ { 1, 15 }, { 2, 7 }, })
  entity:queue_action(action)

  Resources.play_audio(SFX)
end
