use super::plugin_wrapper::PluginWrapper;
use super::server::Server;
use super::ServerConfig;
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

    pub fn with_plugin_interface(mut self, plugin_interface: Box<dyn PluginInterface>) -> Self {
        self.plugin_wrapper.add_plugin_interface(plugin_interface);
        self
    }

    pub async fn start(self) -> Result<(), Box<dyn std::error::Error>> {
        let addr = format!("0.0.0.0:{}", self.config.port);
        let socket = UdpSocket::bind(addr)?;

        socket.take_error()?;

        log::info!("Server listening on: {}", self.config.port);

        let (message_sender, message_receiver) = flume::unbounded();

        Server::new(self.config, socket, self.plugin_wrapper, message_sender)
            .start(message_receiver)
            .await
    }
}
