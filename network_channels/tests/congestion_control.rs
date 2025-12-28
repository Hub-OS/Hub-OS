use network_channels::{
    ChannelSender, Config, ConnectionBuilder, Instant, PacketReceiver, PacketSender, Reliability,
    serialize,
};
use std::{sync::Arc, time::Duration};

fn create_connection(config: &Config) -> (ChannelSender<()>, PacketSender<()>, PacketReceiver<()>) {
    let mut builder = ConnectionBuilder::new(config);
    let channel = builder.bidirectional_channel(());
    let (sender, receiver) = builder.build().split();
    (channel, sender, receiver)
}

#[test]
fn high_latency() {
    let default_config = Config::default();
    let mtu = default_config.mtu as usize;
    let config = Config {
        initial_bytes_per_second: mtu,
        // avoid resending for this test
        rtt_resend_factor: 10000.0,
        ..default_config
    };

    const TICKS_PER_SECOND: u32 = 20;
    const TICK_RTTS: &[u32] = &[1, 5, 20, 30, 40, 50, 100];
    const SECONDS: u32 = 10;

    let mut time = Instant::now();

    for &tick_rtt in TICK_RTTS {
        // A is the sender
        let (channel_a, mut sender_a, mut receiver_a) = create_connection(&config);
        // B is listening and sending acks on the other side
        let (_, mut sender_b, mut receiver_b) = create_connection(&config);

        // send messages
        let serialized = serialize(vec![0; 1330]);
        let message = Arc::new(serialized);

        for _ in 0..1000 {
            channel_a.send_shared_bytes(Reliability::Reliable, message.clone());
        }

        // the test
        let mut messages_sent = 0;

        for tick in 0..(TICKS_PER_SECOND * SECONDS) {
            sender_a.tick(time, |bytes| {
                assert!(bytes.len() > 500, "should not fragment for this test");

                // receive packets so we can send acks
                receiver_b
                    .receive_packet(time, bytes)
                    .expect("valid packet")
                    .expect("valid channel");

                messages_sent += 1;
            });

            if tick == 0 {
                assert!(messages_sent <= 1, "should start at the lowest limit");
            }

            if tick % tick_rtt == 0 && tick > 0 {
                // send acks
                sender_b.tick(time, |bytes| {
                    let _ = receiver_a.receive_packet(time, bytes);
                });
            }

            // pass time
            time += Duration::from_secs(1) / TICKS_PER_SECOND;
        }

        let total_round_trips = TICKS_PER_SECOND * SECONDS / tick_rtt;
        let min_messages = total_round_trips * 3;
        assert!(
            messages_sent > min_messages,
            "should increase send rate. tick_rtt: {tick_rtt}, messages_sent: {messages_sent}, min_messages: {min_messages}"
        );
    }
}
