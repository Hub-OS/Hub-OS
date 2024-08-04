local StandardEnemyAux = {}

function StandardEnemyAux.new()
    return AuxProp.new():declare_immunity(Hit.Flash)
end

return StandardEnemyAux
