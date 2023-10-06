use num_traits::Signed;
use std::borrow::Cow;
use std::ops::{Add, Div, Mul, Rem, Sub};
use std::str::FromStr;

#[derive(Clone)]
pub struct MathExpr<N, V> {
    nodes: Vec<MathExprNode<N, V>>,
}

#[derive(Clone)]
enum MathExprNode<N, V> {
    Number(N),
    Variable(V),
    Abs(usize),
    Negate(usize),
    Sign(usize),
    Add(usize, usize),
    Sub(usize, usize),
    Mul(usize, usize),
    Div(usize, usize),
    Mod(usize, usize),
    Min(usize, usize),
    Max(usize, usize),
    Clamp(usize, usize, usize),
}

impl<N, V> MathExprNode<N, V> {
    fn rebase(&mut self, base: usize) {
        match self {
            MathExprNode::Number(_) | MathExprNode::Variable(_) => {}
            // unary
            MathExprNode::Abs(a_index)
            | MathExprNode::Negate(a_index)
            | MathExprNode::Sign(a_index) => *a_index += base,
            // binary
            MathExprNode::Add(a_index, b_index)
            | MathExprNode::Sub(a_index, b_index)
            | MathExprNode::Mul(a_index, b_index)
            | MathExprNode::Div(a_index, b_index)
            | MathExprNode::Mod(a_index, b_index)
            | MathExprNode::Min(a_index, b_index)
            | MathExprNode::Max(a_index, b_index) => {
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

    pub fn abs(mut self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.nodes.push(MathExprNode::Abs(a_index));
        self
    }

    pub fn negate(mut self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.nodes.push(MathExprNode::Negate(a_index));
        self
    }

    pub fn sign(mut self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.nodes.push(MathExprNode::Sign(a_index));
        self
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

    pub fn add_expr(mut self, expr: Self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.adopt_nodes(expr);
        let b_index = self.nodes.len() - 1;
        self.nodes.push(MathExprNode::Add(a_index, b_index));
        self
    }

    pub fn sub_expr(mut self, expr: Self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.adopt_nodes(expr);
        let b_index = self.nodes.len() - 1;
        self.nodes.push(MathExprNode::Sub(a_index, b_index));
        self
    }

    pub fn mul_expr(mut self, expr: Self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.adopt_nodes(expr);
        let b_index = self.nodes.len() - 1;
        self.nodes.push(MathExprNode::Mul(a_index, b_index));
        self
    }

    pub fn div_expr(mut self, expr: Self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.adopt_nodes(expr);
        let b_index = self.nodes.len() - 1;
        self.nodes.push(MathExprNode::Div(a_index, b_index));
        self
    }

    pub fn min(mut self, b_expr: Self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.adopt_nodes(b_expr);
        let b_index = self.nodes.len() - 1;

        self.nodes.push(MathExprNode::Min(a_index, b_index));
        self
    }

    pub fn max(mut self, b_expr: Self) -> Self {
        let a_index = self.nodes.len() - 1;
        self.adopt_nodes(b_expr);
        let b_index = self.nodes.len() - 1;

        self.nodes.push(MathExprNode::Max(a_index, b_index));
        self
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
            MathExprNode::Abs(a_index) => self.eval_branch(*a_index, var).abs(),
            MathExprNode::Negate(a_index) => -self.eval_branch(*a_index, var),
            MathExprNode::Sign(a_index) => self.eval_branch(*a_index, var).signum(),
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
            MathExprNode::Min(a_index, b_index) => {
                let a = self.eval_branch(*a_index, var);
                let b = self.eval_branch(*b_index, var);

                if a < b {
                    a
                } else {
                    b
                }
            }
            MathExprNode::Max(a_index, b_index) => {
                let a = self.eval_branch(*a_index, var);
                let b = self.eval_branch(*b_index, var);

                if a > b {
                    a
                } else {
                    b
                }
            }
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

use nom::branch::alt;
use nom::bytes::complete::tag;
use nom::character::complete::{alphanumeric1, char, space0 as space};
use nom::combinator::{all_consuming, map_parser};
use nom::error::VerboseError as NomError;
use nom::multi::fold;
use nom::number::complete::recognize_float;
use nom::sequence::{delimited, pair};
use nom::{Finish, IResult, Parser};

fn parse_failure<T>() -> nom::Err<NomError<T>, NomError<T>> {
    nom::Err::Failure(NomError { errors: Vec::new() })
}

impl<N, V> MathExpr<N, V>
where
    N: FromStr,
    V: FromStr,
{
    fn parse_number(i: &str) -> IResult<&str, Self, NomError<&str>> {
        nom::error::context(
            "number",
            map_parser(
                delimited(space, recognize_float, space),
                |s| -> IResult<&str, Self, NomError<&str>> {
                    let number = FromStr::from_str(s).map_err(|_| parse_failure())?;

                    Ok((&s[s.len()..], Self::from_number(number)))
                },
            ),
        )
        .parse(i)
    }

    fn parse_variable(i: &str) -> IResult<&str, Self, NomError<&str>> {
        nom::error::context(
            "variable",
            map_parser(
                delimited(space, alphanumeric1, space),
                |s| -> IResult<&str, Self, NomError<&str>> {
                    let variable = FromStr::from_str(s).map_err(|_| parse_failure())?;

                    Ok((&s[s.len()..], Self::from_variable(variable)))
                },
            ),
        )
        .parse(i)
    }

    fn parse_args(i: &str) -> IResult<&str, Vec<Self>, NomError<&str>> {
        let (i, first_arg) = Self::parse_sub_expr(i)?;
        let mut args = vec![first_arg];

        fold(
            0..,
            pair(delimited(space, char(','), space), Self::parse_sub_expr),
            move || std::mem::take(&mut args),
            |mut acc, (_, expr)| {
                acc.push(expr);
                acc
            },
        )
        .parse(i)
    }

    fn parse_function_inner(i: &str) -> IResult<&str, Self, NomError<&str>> {
        let (i, (name, args)) = pair(
            delimited(space, alphanumeric1, space),
            delimited(
                delimited(space, tag("("), space),
                Self::parse_args,
                delimited(space, tag(")"), space),
            ),
        )
        .parse(i)?;

        match name {
            "min" => {
                let Ok([a, b]) = <[Self; 2]>::try_from(args) else {
                    return Err(parse_failure());
                };

                Ok((i, a.min(b)))
            }
            "max" => {
                let Ok([a, b]) = <[Self; 2]>::try_from(args) else {
                    return Err(parse_failure());
                };

                Ok((i, a.max(b)))
            }
            "clamp" => {
                let Ok([a, b, c]) = <[Self; 3]>::try_from(args) else {
                    return Err(parse_failure());
                };

                Ok((i, a.clamp(b, c)))
            }
            "abs" => {
                let Ok([a]) = <[Self; 1]>::try_from(args) else {
                    return Err(parse_failure());
                };

                Ok((i, a.abs()))
            }
            "sign" => {
                let Ok([a]) = <[Self; 1]>::try_from(args) else {
                    return Err(parse_failure());
                };

                Ok((i, a.sign()))
            }
            _ => Err(parse_failure()),
        }
    }

    fn parse_function(i: &str) -> IResult<&str, Self, NomError<&str>> {
        nom::error::context("function call", Self::parse_function_inner).parse(i)
    }

    fn parse_parens(i: &str) -> IResult<&str, Self, NomError<&str>> {
        delimited(
            space,
            delimited(tag("("), Self::parse_sub_expr, tag(")")),
            space,
        )
        .parse(i)
    }

    fn parse_factor(i: &str) -> IResult<&str, Self, NomError<&str>> {
        alt((
            Self::parse_number,
            Self::parse_function,
            Self::parse_variable,
            Self::parse_parens,
        ))
        .parse(i)
    }

    fn parse_negate(i: &str) -> (&str, bool) {
        let parse = || -> IResult<&str, char> { delimited(space, char('-'), space).parse(i) };

        match parse() {
            Ok((i, _)) => (i, true),
            Err(_) => (i, false),
        }
    }

    fn parse_term(i: &str) -> IResult<&str, Self, NomError<&str>> {
        let (i, negate) = Self::parse_negate(i);
        let (i, mut init) = Self::parse_factor(i)?;

        let (i, expr) = fold(
            0..,
            pair(alt((char('*'), char('/'))), Self::parse_factor),
            move || Self {
                nodes: std::mem::take(&mut init.nodes),
            },
            |acc, (op, expr)| {
                if op == '*' {
                    acc.mul_expr(expr)
                } else {
                    acc.div_expr(expr)
                }
            },
        )
        .parse(i)?;

        let expr = if negate { expr.negate() } else { expr };

        Ok((i, expr))
    }

    pub fn parse_sub_expr(i: &str) -> IResult<&str, Self, NomError<&str>> {
        let (i, mut init) = Self::parse_term(i)?;

        fold(
            0..,
            pair(alt((char('+'), char('-'))), Self::parse_term),
            move || Self {
                nodes: std::mem::take(&mut init.nodes),
            },
            |acc, (op, expr)| {
                if op == '+' {
                    acc.add_expr(expr)
                } else {
                    acc.sub_expr(expr)
                }
            },
        )
        .parse(i)
    }

    pub fn parse_full(i: &str) -> IResult<&str, Self, NomError<&str>> {
        all_consuming(Self::parse_sub_expr).parse(i)
    }

    pub fn parse(source: &str) -> Result<MathExpr<N, V>, String> {
        match Self::parse_full(source).finish() {
            Ok((_, expr)) => Ok(expr),
            Err(err) => Err(nom::error::convert_error(source, err)),
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
                MathExprNode::Negate(a_index) => {
                    let completed = arg_index > 0;

                    if !completed {
                        strings.push(Cow::Borrowed("-"));
                        work.push((*a_index, 0));
                    }

                    completed
                }
                MathExprNode::Sign(a_index) => {
                    write_function(&mut work, &mut strings, "sign", &[*a_index], arg_index)
                }
                MathExprNode::Min(a_index, b_index) => write_function(
                    &mut work,
                    &mut strings,
                    "min",
                    &[*a_index, *b_index],
                    arg_index,
                ),
                MathExprNode::Max(a_index, b_index) => write_function(
                    &mut work,
                    &mut strings,
                    "max",
                    &[*a_index, *b_index],
                    arg_index,
                ),
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

    #[test]
    fn parse() -> Result<(), String> {
        let undershirt_expr =
            MathExpr::<f32, AuxVariable>::parse("clamp(DAMAGE, 1.0, HEALTH - 1.0)")?;
        assert_eq!(
            format!("{undershirt_expr:?}"),
            "clamp(Damage, 1.0, (Health - 1.0))"
        );

        type SimpleExpr = MathExpr<i32, i32>;

        let example_expr = SimpleExpr::parse("3 + 2 - 1")?;
        assert_eq!(format!("{example_expr:?}"), "((3 + 2) - 1)");

        // incomplete expression
        assert!(SimpleExpr::parse("3+").is_err());

        // signed
        assert_eq!(SimpleExpr::parse("-1")?.eval(|_| unreachable!()), -1);
        assert_eq!(SimpleExpr::parse("+1")?.eval(|_| unreachable!()), 1);

        // order of operations
        assert_eq!(SimpleExpr::parse("3-1*4")?.eval(|_| unreachable!()), -1);
        assert_eq!(SimpleExpr::parse("(3-1)*4")?.eval(|_| unreachable!()), 8);

        assert_eq!(
            SimpleExpr::parse("-clamp(4, 1, 2) - 1 * 3")?.eval(|_| unreachable!()),
            -5
        );

        Ok(())
    }
}
