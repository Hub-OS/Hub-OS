use indexmap::IndexMap;

pub struct CommandInfo {
    pub description: String,
}

#[derive(Default)]
pub struct CommandRegistry {
    commands: IndexMap<String, CommandInfo>,
}

impl CommandRegistry {
    pub fn register_command(&mut self, name: String, info: CommandInfo) {
        self.commands.insert(name, info);
    }

    pub fn contains_command(&self, name: &str) -> bool {
        self.commands.contains_key(name)
    }

    pub fn get_command_info(&self, name: &str) -> Option<&CommandInfo> {
        self.commands.get(name)
    }

    pub fn commands(&self) -> impl Iterator<Item = (&str, &CommandInfo)> {
        self.commands.iter().map(|(k, v)| (k.as_str(), v))
    }

    pub fn len(&self) -> usize {
        self.commands.len()
    }
}
