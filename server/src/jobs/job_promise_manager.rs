use super::job_promise::JobPromise;
use std::collections::HashMap;

pub struct JobPromiseManager {
    promises: HashMap<usize, JobPromise>,
    next_id: usize,
}

impl JobPromiseManager {
    pub fn new() -> JobPromiseManager {
        JobPromiseManager {
            promises: HashMap::new(),
            next_id: 0,
        }
    }

    pub fn get_promise(&self, id: usize) -> Option<&JobPromise> {
        self.promises.get(&id)
    }

    pub fn get_promise_mut(&mut self, id: usize) -> Option<&mut JobPromise> {
        self.promises.get_mut(&id)
    }

    pub fn add_promise(&mut self, promise: JobPromise) -> usize {
        let id = self.next_id;

        self.promises.insert(id, promise);

        self.next_id += 1;

        id
    }

    pub fn remove_promise(&mut self, id: usize) {
        self.promises.remove(&id);

        if self.promises.is_empty() {
            self.next_id = 0;
        }
    }
}
