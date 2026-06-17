use hub_os::framework::logging::log;
use hub_os::framework::winit_game_loop::WinitPlatformApp;
use hub_os::framework::winit_game_loop::android;

use android::content::AndroidContext;
use android::{AndroidJVM, jni};

pub struct AndroidNetworkLock {
    _lock_object: jni::objects::Global<jni::objects::JObject<'static>>,
}

// API level 29
const WIFI_MODE_FULL_LOW_LATENCY: jni::sys::jint = 4;

pub fn acquire_multicast_lock(app: &WinitPlatformApp) -> Option<AndroidNetworkLock> {
    let vm = AndroidJVM::from(app);

    let mut multicast_lock_result = None;

    vm.wrap(|jni_env| {
        log::trace!("Acquiring Android MulticastLock");

        use jni::objects::JObject;

        let context = AndroidContext::from_app(jni_env, app);
        let wifi_manager = context.wifi_service(jni_env)?;

        let multicast_tag = jni_env.new_string("hub_os_multicast")?;
        let multicast_lock = wifi_manager.create_multicast_lock(jni_env, &multicast_tag)?;

        multicast_lock.acquire(jni_env)?;

        let multicast_lock_object: JObject = multicast_lock.into();

        multicast_lock_result = Some(AndroidNetworkLock {
            _lock_object: jni_env.new_global_ref(multicast_lock_object)?,
        });

        log::trace!("Acquired MulticastLock");

        Ok(())
    });

    multicast_lock_result
}

pub fn acquire_low_latency_lock(app: &WinitPlatformApp) -> Option<AndroidNetworkLock> {
    let vm = AndroidJVM::from(app);

    let mut wifi_lock_result = None;

    vm.wrap(|jni_env| {
        use jni::objects::JObject;

        log::trace!("Acquiring Low Latency WifiLock");

        let context = AndroidContext::from_app(jni_env, app);
        let wifi_manager = context.wifi_service(jni_env)?;

        let lock_tag = jni_env.new_string("hub_os_low_latency")?;
        let wifi_lock =
            wifi_manager.create_wifi_lock(jni_env, WIFI_MODE_FULL_LOW_LATENCY, &lock_tag)?;

        wifi_lock.acquire(jni_env)?;

        let wifi_lock_object: JObject = wifi_lock.into();

        wifi_lock_result = Some(AndroidNetworkLock {
            _lock_object: jni_env.new_global_ref(&wifi_lock_object)?,
        });

        log::trace!("Acquired Low Latency WifiLock");

        Ok(())
    });

    wifi_lock_result
}
