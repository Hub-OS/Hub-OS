use instant::Duration;

pub async fn sleep(duration: Duration) {
    async_io::Timer::after(duration).await;
}
