local TextStyle = {}

function TextStyle.new(font, texture_path, animation_path)
  local t = {
    font = font,
    texture_path = texture_path,
    animation_path = animation_path,
  }
  setmetatable(t, TextStyle)
  return t
end

function TextStyle.new_monospace(...)
  local t = TextStyle.new(...)
  t.monospace = true
  return t
end

return TextStyle
