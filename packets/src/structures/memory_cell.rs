use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, PartialEq)]
pub enum MemoryCell {
    Integer(i64),
    Float(f64),
    String(String),
}

impl Eq for MemoryCell {}

impl std::fmt::Debug for MemoryCell {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Integer(v) => write!(f, "{v}"),
            Self::Float(v) => write!(f, "{v}"),
            Self::String(v) => write!(f, "{v}"),
        }
    }
}

impl From<i64> for MemoryCell {
    fn from(value: i64) -> Self {
        Self::Integer(value)
    }
}

impl From<f64> for MemoryCell {
    fn from(value: f64) -> Self {
        Self::Float(value)
    }
}

impl From<String> for MemoryCell {
    fn from(value: String) -> Self {
        Self::String(value)
    }
}

impl From<&str> for MemoryCell {
    fn from(value: &str) -> Self {
        Self::String(value.to_string())
    }
}
