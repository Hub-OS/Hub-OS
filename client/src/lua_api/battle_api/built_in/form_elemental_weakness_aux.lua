local FormElementWeaknessAux = {}

function FormElementWeaknessAux.new_cleanser(element)
    return AuxProp.new()
        :require_hit_element(element)
        :require_hit_damage(Compare.GT, 0)
        :with_callback()
end

function FormElementWeaknessAux.new()
    return AuxProp.new()
        :require_hit_element_is_weakness()
        :require_hit_damage(Compare.GT, 0)
        :deactivate_form()
end

return FormElementWeaknessAux
