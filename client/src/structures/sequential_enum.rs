/// Creates an enum with a specified representation that
/// implements Copy, Eq, Ord, and related traits.
///
/// Usage Example:
/// ```rust
/// use hub_os::sequential_enum;
///
/// sequential_enum! {
///     pub MyEnum: i8 {
///         A,
///         B
///     }
/// }
/// ```
#[macro_export]
macro_rules! sequential_enum {
    ($vis: vis $name:ident: $repr: ty { $($variant: ident),* }) => {
        #[derive(Clone, Copy, PartialEq, Eq)]
        #[repr($repr)]
        $vis enum $name {
            $($variant,)*
        }

        impl std::cmp::Ord for $name {
            fn cmp(&self, other: &Self) -> std::cmp::Ordering {
                (*self as u8).cmp(&(*other as u8))
            }
        }

        impl std::cmp::PartialOrd for $name {
            fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
                Some(self.cmp(other))
            }
        }
    }
}

pub use sequential_enum;
