use serde::{de::DeserializeOwned, Serialize};

pub trait Label: Copy + Eq + Serialize + DeserializeOwned + 'static {}

impl<T: Copy + Eq + Serialize + DeserializeOwned + 'static> Label for T {}
