local rule = DefenseRule.new(DefensePriority.Body, DefenseOrder.CollisionOnly)

rule.filter_statuses_func = function(props)
    props.flags = props.flags & ~Hit.Flinch & ~Hit.Flash
    return props
end

return rule
