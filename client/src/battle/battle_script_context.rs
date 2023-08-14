use super::{BattleSimulation, SharedBattleResources};
use framework::prelude::GameIO;

pub struct BattleScriptContext<'a> {
    pub vm_index: usize,
    pub resources: &'a SharedBattleResources,
    pub game_io: &'a GameIO,
    pub simulation: &'a mut BattleSimulation,
}
