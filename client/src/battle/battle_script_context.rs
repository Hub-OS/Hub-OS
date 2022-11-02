use super::{BattleSimulation, RollbackVM};
use crate::resources::Globals;
use framework::prelude::GameIO;

pub struct BattleScriptContext<'a> {
    pub vm_index: usize,
    pub vms: &'a [RollbackVM],
    pub game_io: &'a GameIO<Globals>,
    pub simulation: &'a mut BattleSimulation,
}
