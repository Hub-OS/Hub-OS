use super::GenerationalIndex;

// best for random access
pub type SlotMap<V> = slotmap::SlotMap<GenerationalIndex, V>;

// good for iteration, good for random access
pub type HopSlotMap<V> = slotmap::HopSlotMap<GenerationalIndex, V>;

// best for iteration, bad random access
pub type DenseSlotMap<V> = slotmap::DenseSlotMap<GenerationalIndex, V>;
