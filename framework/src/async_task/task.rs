#[must_use = "tasks are cancelled when dropped, use `.detach()` to run the task in the background"]
pub struct AsyncTask<T> {
    task: async_task::Task<T>,
}

impl<T> AsyncTask<T> {
    pub fn new(task: async_task::Task<T>) -> Self {
        Self { task }
    }

    pub fn is_finished(&self) -> bool {
        self.task.is_finished()
    }

    /// Cancels the task and returns the result if it's already finished
    pub fn join(self) -> Option<T> {
        if self.is_finished() {
            Some(pollster::block_on(self.task.cancel()).unwrap())
        } else {
            None
        }
    }

    /// Run the task without requiring the result to be used
    pub fn detach(self) {
        self.task.detach()
    }
}
