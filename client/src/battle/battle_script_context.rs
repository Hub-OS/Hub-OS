use super::{BattleSimulation, RollbackVM};
use framework::prelude::GameIO;

pub struct BattleScriptContext<'a> {
    pub vm_index: usize,
    pub vms: &'a [RollbackVM],
    pub game_io: &'a GameIO,
    pub simulation: &'a mut BattleSimulation,
}
