use super::ServerConfig;
use super::plugin_wrapper::PluginWrapper;
use super::server::Server;
use crate::plugins::PluginInterface;
use std::net::UdpSocket;

pub struct ServerBuilder {
    config: ServerConfig,
    plugin_wrapper: PluginWrapper,
}

impl ServerBuilder {
    pub fn new(config: ServerConfig) -> Self {
        Self {
            config,
            plugin_wrapper: PluginWrapper::new(),
        }
    }

    pub fn with_plugin(mut self, plugin_interface: Box<dyn PluginInterface>) -> Self {
        self.plugin_wrapper.add_plugin_interface(plugin_interface);
        self
    }

    pub async fn start(self) -> Result<(), Box<dyn std::error::Error>> {
        use std::net::{Ipv6Addr, SocketAddrV6};

        let socket = socket2::Socket::new(
            socket2::Domain::IPV6,
            socket2::Type::DGRAM,
            Some(socket2::Protocol::UDP),
        )?;

        // disable ipv6 only to allow connection to IPv4 endpoints
        if let Err(err) = socket.set_only_v6(false) {
            log::warn!(
                "Failed to disable IPV6_V6ONLY, players may be unable to connect using IPv4 addresses: {err:?}"
            );
        };

        let addr = SocketAddrV6::new(Ipv6Addr::UNSPECIFIED, self.config.args.port, 0, 0);
        socket.bind(&addr.into())?;

        let socket: UdpSocket = socket.into();
        socket.take_error()?;

        log::info!("Server listening on: {}", self.config.args.port);

        let (message_sender, message_receiver) = flume::unbounded();

        Server::new(self.config, socket, self.plugin_wrapper, message_sender)
            .start(message_receiver)
            .await
    }
}
