use std::future::Future;

/// Value exists, but can be invalid.
pub struct SyncResultAsyncError<T, E, F>
where
    F: Future<Output = Option<E>> + Send,
{
    value: T,
    future_error: F,
}

impl<T, E, F> SyncResultAsyncError<T, E, F>
where
    F: Future<Output = Option<E>> + Send,
{
    pub fn new(value: T, future_error: F) -> Self {
        Self {
            value,
            future_error,
        }
    }

    /// Won't panic as the error is unknown until the future is resolved, but may cause panics when the value is used
    pub fn unwrap(self) -> T {
        self.value
    }

    /// Convert into a normal Result
    pub async fn result(self) -> Result<T, E> {
        match self.future_error.await {
            Some(e) => Err(e),
            None => Ok(self.value),
        }
    }
}
