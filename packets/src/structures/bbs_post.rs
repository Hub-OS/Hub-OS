use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct BbsPost {
    pub id: String,
    pub read: bool,
    pub title: String,
    pub author: String,
}
