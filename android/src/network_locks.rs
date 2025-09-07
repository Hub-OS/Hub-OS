use android::{jni, AndroidJVM};
use hub_os::framework::logging::log;
use hub_os::framework::winit_game_loop::android;
use hub_os::framework::winit_game_loop::WinitPlatformApp;

pub struct AndroidWifiLock {
    _lock_object: jni::objects::GlobalRef,
}

// API 29
const WIFI_MODE_FULL_LOW_LATENCY: jni::sys::jint = 4;

pub fn acquire_multicast_lock(app: &WinitPlatformApp) -> Option<AndroidWifiLock> {
    let vm = AndroidJVM::from(app);

    let mut multicast_lock = None;

    vm.wrap(|jni_env| {
        log::trace!("Acquiring Android MulticastLock");

        use jni::objects::JObject;

        let activity_object =
            unsafe { JObject::from_raw(std::mem::transmute(app.activity_as_ptr())) };

        // API 1
        let wifi_service_name = jni_env.get_static_field(
            "android/content/Context",
            "WIFI_SERVICE",
            "Ljava/lang/String;",
        )?;

        // API 1
        let wifi_object: JObject = jni_env
            .call_method(
                &activity_object,
                "getSystemService",
                "(Ljava/lang/String;)Ljava/lang/Object;",
                &[(&wifi_service_name).into()],
            )?
            .try_into()?;

        // API 4
        let multicast_tag = jni_env.new_string("hub_os_multicast")?;
        let multicast_lock_object: JObject = jni_env
            .call_method(
                &wifi_object,
                "createMulticastLock",
                "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;",
                &[(&multicast_tag).into()],
            )?
            .try_into()?;

        // API 4
        jni_env.call_method(&multicast_lock_object, "acquire", "()V", &[])?;

        multicast_lock = Some(AndroidWifiLock {
            _lock_object: jni_env.new_global_ref(&multicast_lock_object)?,
        });

        log::trace!("Acquired MulticastLock");

        Ok(())
    });

    multicast_lock
}

pub fn acquire_low_latency_lock(app: &WinitPlatformApp) -> Option<AndroidWifiLock> {
    let vm = AndroidJVM::from(app);

    let mut wifi_lock = None;

    vm.wrap(|jni_env| {
        use jni::objects::JObject;

        log::trace!("Acquiring Low Latency WifiLock");

        let activity_object =
            unsafe { JObject::from_raw(std::mem::transmute(app.activity_as_ptr())) };

        // API 1
        let wifi_service_name = jni_env.get_static_field(
            "android/content/Context",
            "WIFI_SERVICE",
            "Ljava/lang/String;",
        )?;

        // API 1
        let wifi_object: JObject = jni_env
            .call_method(
                &activity_object,
                "getSystemService",
                "(Ljava/lang/String;)Ljava/lang/Object;",
                &[(&wifi_service_name).into()],
            )?
            .try_into()?;

        // API 3
        let lock_tag = jni_env.new_string("hub_os_low_latency")?;
        let multicast_lock_object: JObject = jni_env
            .call_method(
                &wifi_object,
                "createWifiLock",
                "(ILjava/lang/String;)Landroid/net/wifi/WifiManager$WifiLock;",
                &[WIFI_MODE_FULL_LOW_LATENCY.into(), (&lock_tag).into()],
            )?
            .try_into()?;

        // API 1
        jni_env.call_method(&multicast_lock_object, "acquire", "()V", &[])?;

        wifi_lock = Some(AndroidWifiLock {
            _lock_object: jni_env.new_global_ref(&multicast_lock_object)?,
        });

        log::trace!("Acquired Low Latency WifiLock");

        Ok(())
    });

    wifi_lock
}
