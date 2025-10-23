use packets::structures::MemoryCell;
use std::collections::HashMap;
use std::sync::{Arc, Mutex};

pub type EntityMemories = FrameCell<HashMap<String, MemoryCell>>;

pub struct FrameCell<T> {
    mutable: bool,
    value: Arc<Mutex<T>>,
}

impl<T> FrameCell<T> {
    pub fn new(value: T) -> Self {
        Self {
            mutable: true, // avoid cloning for mutation on the first frame
            value: Arc::new(Mutex::new(value)),
        }
    }

    pub fn read<R>(&mut self, f: impl FnOnce(&T) -> R) -> R {
        let value = self.value.lock().unwrap();

        f(&value)
    }
}

impl<T: Clone> FrameCell<T> {
    pub fn update(&mut self, f: impl FnOnce(&mut T)) {
        let mut value = self.value.lock().unwrap();

        if !self.mutable {
            // avoid mutating prior clones by cloning at the first mutation
            self.mutable = true;
            *value = (*value).clone();
        }

        f(&mut value);
    }

    pub fn clone_inner(&self) -> T {
        self.value.lock().unwrap().clone()
    }
}

impl<T: Clone> Clone for FrameCell<T> {
    fn clone(&self) -> Self {
        Self {
            mutable: false, // unset to force clones on next mutation
            value: self.value.clone(),
        }
    }
}
