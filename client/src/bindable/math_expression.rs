use num_traits::Signed;
use std::borrow::Cow;
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

impl<N, V> MathExprNode<N, V> {
    fn rebase(&mut self, base: usize) {
        match self {
            MathExprNode::Number(_) | MathExprNode::Variable(_) => {}
            // unary
            MathExprNode::Abs(a_index) | MathExprNode::Sign(a_index) => *a_index += base,
            // binary
            MathExprNode::Add(a_index, b_index)
            | MathExprNode::Sub(a_index, b_index)
            | MathExprNode::Mul(a_index, b_index)
            | MathExprNode::Div(a_index, b_index)
            | MathExprNode::Mod(a_index, b_index) => {
                *a_index += base;
                *b_index += base;
            }
            // ternary
            MathExprNode::Clamp(a_index, b_index, c_index) => {
                *a_index += base;
                *b_index += base;
                *c_index += base;
            }
        }
    }
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

    fn adopt_nodes(&mut self, mut expr: Self) {
        let base_index = self.nodes.len();
        for node in &mut expr.nodes {
            node.rebase(base_index);
        }

        self.nodes.extend(expr.nodes);
    }

    pub fn clamp(mut self, b_expr: Self, c_expr: Self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.adopt_nodes(b_expr);
        let b_index = self.nodes.len() - 1;
        self.adopt_nodes(c_expr);
        let c_index = self.nodes.len() - 1;

        self.nodes
            .push(MathExprNode::Clamp(a_index, b_index, c_index));
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

impl<N, V> std::fmt::Debug for MathExpr<N, V>
where
    N: std::fmt::Debug,
    V: std::fmt::Debug,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut strings = Vec::new();
        let mut work = vec![(self.nodes.len() - 1, 0)];

        fn write_args<'a>(
            work: &mut Vec<(usize, usize)>,
            strings: &mut Vec<Cow<'a, str>>,
            separator: &'a str,
            arg_node_indices: &[usize],
            arg_index: usize,
        ) -> bool {
            let Some(&index) = arg_node_indices.get(arg_index) else {
                return true;
            };

            if arg_index > 0 {
                strings.push(Cow::Borrowed(separator));
            }

            work.push((index, 0));
            false
        }

        fn wrap_parens<'a>(
            strings: &mut Vec<Cow<'a, str>>,
            arg_index: usize,
            mut inner: impl FnMut(&mut Vec<Cow<'a, str>>) -> bool,
        ) -> bool {
            if arg_index == 0 {
                strings.push(Cow::Borrowed("("));
            }

            let complete = inner(strings);

            if complete {
                strings.push(Cow::Borrowed(")"));
            }

            complete
        }

        fn write_function<'a>(
            work: &mut Vec<(usize, usize)>,
            strings: &mut Vec<Cow<'a, str>>,
            name: &'a str,
            arg_node_indices: &[usize],
            arg_index: usize,
        ) -> bool {
            if arg_index == 0 {
                strings.push(Cow::Borrowed(name));
            }

            wrap_parens(strings, arg_index, |strings| {
                write_args(work, strings, ", ", arg_node_indices, arg_index)
            })
        }

        loop {
            let Some((node_index, arg_index)) = work.last_mut().cloned() else {
                break;
            };

            let complete = match &self.nodes[node_index] {
                MathExprNode::Number(n) => {
                    strings.push(Cow::Owned(format!("{n:?}")));
                    true
                }
                MathExprNode::Variable(v) => {
                    strings.push(Cow::Owned(format!("{v:?}")));
                    true
                }
                MathExprNode::Add(a_index, b_index) => {
                    wrap_parens(&mut strings, arg_index, |strings| {
                        write_args(&mut work, strings, " + ", &[*a_index, *b_index], arg_index)
                    })
                }
                MathExprNode::Sub(a_index, b_index) => {
                    wrap_parens(&mut strings, arg_index, |strings| {
                        write_args(&mut work, strings, " - ", &[*a_index, *b_index], arg_index)
                    })
                }
                MathExprNode::Mul(a_index, b_index) => write_args(
                    &mut work,
                    &mut strings,
                    " * ",
                    &[*a_index, *b_index],
                    arg_index,
                ),
                MathExprNode::Div(a_index, b_index) => write_args(
                    &mut work,
                    &mut strings,
                    " / ",
                    &[*a_index, *b_index],
                    arg_index,
                ),
                MathExprNode::Mod(a_index, b_index) => write_args(
                    &mut work,
                    &mut strings,
                    " % ",
                    &[*a_index, *b_index],
                    arg_index,
                ),
                MathExprNode::Abs(a_index) => {
                    write_function(&mut work, &mut strings, "abs", &[*a_index], arg_index)
                }
                MathExprNode::Sign(a_index) => {
                    write_function(&mut work, &mut strings, "sign", &[*a_index], arg_index)
                }
                MathExprNode::Clamp(a_index, b_index, c_index) => write_function(
                    &mut work,
                    &mut strings,
                    "clamp",
                    &[*a_index, *b_index, *c_index],
                    arg_index,
                ),
            };

            if complete {
                work.pop();

                if let Some((_, arg_index)) = work.last_mut() {
                    *arg_index += 1;
                }
            }
        }

        f.write_fmt(format_args!("{}", strings.join("")))
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

#[cfg(test)]
mod test {
    use super::*;
    use crate::bindable::AuxVariable;

    fn undershirt_expr() -> MathExpr<f64, AuxVariable> {
        // clamp(DAMAGE, 1.0, HEALTH - 1.0)
        MathExpr::from_variable(AuxVariable::Damage).clamp(
            MathExpr::from_number(1.0),
            MathExpr::from_variable(AuxVariable::Health).sub(1.0),
        )
    }

    fn example_expr() -> MathExpr<i32, AuxVariable> {
        // 3 + 2 - 1
        MathExpr::from_number(3).add(2).sub(1)
    }

    #[test]
    fn format() {
        let undershirt_expr = undershirt_expr();

        assert_eq!(
            format!("{undershirt_expr:?}"),
            "clamp(Damage, 1.0, (Health - 1.0))"
        );

        let example_expr = example_expr();

        assert_eq!(format!("{example_expr:?}"), "((3 + 2) - 1)");
    }

    #[test]
    fn eval() {
        let undershirt_result = undershirt_expr().eval(|v| match v {
            AuxVariable::MaxHealth => 100.0,
            AuxVariable::Health => 100.0,
            AuxVariable::Damage => 120.0,
        });
        assert_eq!(undershirt_result, 99.0);

        let example_result = example_expr().eval(|_| unreachable!());
        assert_eq!(example_result, 4);
    }
}
