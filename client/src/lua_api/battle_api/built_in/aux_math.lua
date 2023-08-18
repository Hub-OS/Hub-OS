local AuxMath = {}
AuxMath.__index = AuxMath

local ops = {
  add = function(a, b) return a + b end,
  sub = function(a, b) return a - b end,
  mul = function(a, b) return a * b end,
  div = function(a, b) return a / b end,
  mod = function(a, b) return a % b end,
  abs = function(a) return math.abs(a) end,
  sign = function(a) return a > 0 and 1 or a < 0 and -1 or a end,
  clamp = function(a, b, c) return math.min(math.max(a, b), c) end,
}

for op, _ in pairs(ops) do
  AuxMath[op] = function(...)
    local table = { op, ... }
    setmetatable(table, AuxMath)
    return table
  end
end

function AuxMath.eval(expr, variables)
  local expr_type = type(expr)

  if expr_type == "string" then
    return variables[expr]
  elseif expr_type ~= "table" then
    return expr
  end

  local op = expr[1]
  local args = {}

  for i = 2, #expr do
    args[i - 1] = AuxMath.eval(expr[i], variables)
  end

  return ops[op](table.unpack(args))
end

return AuxMath
