local Hitbox = {}

function Hitbox.new(team, damage)
    local spell = Spell.new(team)
    spell:set_hit_props(HitProps.new(
        damage or 0,
        Hit.Flinch | Hit.Impact,
        Element.None,
        spell:context(),
        Drag.None
    ))

    spell.on_update_func = function()
        spell:get_tile():attack_entities(spell)
        spell:delete()
    end

    return spell
end

return Hitbox
