local team, damage = ...

local spell = Battle.Spell.new(team)
spell:set_hit_props(Battle.HitProps.new(
    damage or 0,
    Hit.Flinch | Hit.Impact,
    Element.None,
    spell:get_context(),
    Drag.None
))

spell.on_update_func = function()
    spell:get_tile():attack_entities(spell)
    spell:delete()
end

return spell
