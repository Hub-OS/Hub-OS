use num_traits::Signed;
use std::ops::{Add, Div, Mul, Rem, Sub};

#[derive(Clone)]
pub struct MathExpr<N, V> {
    nodes: Vec<MathExprNode<N, V>>,
}

#[derive(Clone)]
enum MathExprNode<N, V> {
    Number(N),
    Variable(V),
    Add(usize, usize),
    Sub(usize, usize),
    Mul(usize, usize),
    Div(usize, usize),
    Mod(usize, usize),
    Abs(usize),
    Sign(usize),
    Clamp(usize, usize, usize),
}

impl<N, V> MathExpr<N, V> {
    pub fn from_number(n: N) -> Self {
        Self {
            nodes: vec![MathExprNode::Number(n)],
        }
    }

    pub fn from_variable(v: V) -> Self {
        Self {
            nodes: vec![MathExprNode::Variable(v)],
        }
    }

    pub fn add(mut self, n: N) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Number(n));
        self.nodes.push(MathExprNode::Add(a_index, b_index));
        self
    }

    pub fn sub(mut self, n: N) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Number(n));
        self.nodes.push(MathExprNode::Sub(a_index, b_index));
        self
    }

    pub fn mul(mut self, n: N) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Number(n));
        self.nodes.push(MathExprNode::Mul(a_index, b_index));
        self
    }

    pub fn div(mut self, n: N) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Number(n));
        self.nodes.push(MathExprNode::Div(a_index, b_index));
        self
    }

    pub fn add_variable(mut self, v: V) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Variable(v));
        self.nodes.push(MathExprNode::Add(a_index, b_index));
        self
    }

    pub fn sub_variable(mut self, v: V) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Variable(v));
        self.nodes.push(MathExprNode::Sub(a_index, b_index));
        self
    }

    pub fn mul_variable(mut self, v: V) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Variable(v));
        self.nodes.push(MathExprNode::Mul(a_index, b_index));
        self
    }

    pub fn div_variable(mut self, v: V) -> Self {
        let a_index = self.nodes.len() - 1;
        let b_index = self.nodes.len();
        self.nodes.push(MathExprNode::Variable(v));
        self.nodes.push(MathExprNode::Div(a_index, b_index));
        self
    }
}

impl<N, V> MathExpr<N, V>
where
    N: Copy + Signed + Add + Sub + Mul + Div + Rem + PartialOrd,
{
    pub fn eval(&self, var: impl Fn(&V) -> N) -> N {
        self.eval_branch(self.nodes.len() - 1, &var)
    }

    fn eval_branch(&self, index: usize, var: &impl Fn(&V) -> N) -> N {
        match &self.nodes[index] {
            MathExprNode::Number(n) => *n,
            MathExprNode::Variable(v) => var(v),
            MathExprNode::Add(a_index, b_index) => {
                self.eval_branch(*a_index, var) + self.eval_branch(*b_index, var)
            }
            MathExprNode::Sub(a_index, b_index) => {
                self.eval_branch(*a_index, var) - self.eval_branch(*b_index, var)
            }
            MathExprNode::Mul(a_index, b_index) => {
                self.eval_branch(*a_index, var) * self.eval_branch(*b_index, var)
            }
            MathExprNode::Div(a_index, b_index) => {
                self.eval_branch(*a_index, var) / self.eval_branch(*b_index, var)
            }
            MathExprNode::Mod(a_index, b_index) => {
                self.eval_branch(*a_index, var) % self.eval_branch(*b_index, var)
            }
            MathExprNode::Abs(a_index) => self.eval_branch(*a_index, var).abs(),
            MathExprNode::Sign(a_index) => self.eval_branch(*a_index, var).signum(),
            MathExprNode::Clamp(a_index, b_index, c_index) => {
                let value = self.eval_branch(*a_index, var);
                let min = self.eval_branch(*b_index, var);
                let max = self.eval_branch(*c_index, var);

                if value < min {
                    min
                } else if value > max {
                    max
                } else {
                    value
                }
            }
        }
    }
}

impl<'lua, N, V> rollback_mlua::FromLua<'lua> for MathExpr<N, V>
where
    N: rollback_mlua::FromLua<'lua>,
    V: rollback_mlua::FromLua<'lua>,
{
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let mut expression = MathExpr { nodes: Vec::new() };
        push_from_lua(&mut expression, lua, lua_value)?;

        Ok(expression)
    }
}

fn push_from_lua<'lua, N, V>(
    expression: &mut MathExpr<N, V>,
    lua: &'lua rollback_mlua::Lua,
    lua_value: rollback_mlua::Value<'lua>,
) -> rollback_mlua::Result<usize>
where
    N: rollback_mlua::FromLua<'lua>,
    V: rollback_mlua::FromLua<'lua>,
{
    let table = if let Ok(table) = lua.unpack::<rollback_mlua::Table>(lua_value.clone()) {
        table
    } else if let Ok(n) = lua.unpack::<N>(lua_value.clone()) {
        expression.nodes.push(MathExprNode::Number(n));
        return Ok(expression.nodes.len() - 1);
    } else {
        let v = lua.unpack::<V>(lua_value.clone())?;
        expression.nodes.push(MathExprNode::Variable(v));
        return Ok(expression.nodes.len() - 1);
    };

    let op_string: rollback_mlua::String = table.raw_get(1)?;
    let op_str = op_string.to_str()?;

    let node = match op_str {
        "add" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            let b_index = push_from_lua(expression, lua, table.raw_get(3)?)?;
            MathExprNode::Add(a_index, b_index)
        }
        "sub" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            let b_index = push_from_lua(expression, lua, table.raw_get(3)?)?;
            MathExprNode::Sub(a_index, b_index)
        }
        "mul" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            let b_index = push_from_lua(expression, lua, table.raw_get(3)?)?;
            MathExprNode::Mul(a_index, b_index)
        }
        "div" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            let b_index = push_from_lua(expression, lua, table.raw_get(3)?)?;
            MathExprNode::Div(a_index, b_index)
        }
        "mod" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            let b_index = push_from_lua(expression, lua, table.raw_get(3)?)?;
            MathExprNode::Mod(a_index, b_index)
        }
        "abs" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            MathExprNode::Abs(a_index)
        }
        "sign" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            MathExprNode::Sign(a_index)
        }
        "clamp" => {
            let a_index = push_from_lua(expression, lua, table.raw_get(2)?)?;
            let b_index = push_from_lua(expression, lua, table.raw_get(3)?)?;
            let c_index = push_from_lua(expression, lua, table.raw_get(4)?)?;

            MathExprNode::Clamp(a_index, b_index, c_index)
        }
        _ => {
            return Err(rollback_mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "MathExpr",
                message: None,
            });
        }
    };

    expression.nodes.push(node);

    Ok(expression.nodes.len() - 1)
}
