use serde::{Deserialize, Serialize};
use std::collections::VecDeque;

#[derive(Default, PartialEq, Eq, Debug, Clone, Serialize, Deserialize)]
#[serde(transparent)]
pub struct RunLengthDeque<T> {
    queue: VecDeque<(T, usize)>,
}

impl<T> RunLengthDeque<T> {
    pub fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    pub fn len(&self) -> usize {
        self.queue.iter().map(|(_, count)| count).sum()
    }

    pub fn get(&self, mut index: usize) -> Option<&T> {
        self.queue
            .iter()
            .find(move |(_, count)| {
                if *count > index {
                    return true;
                }

                index -= *count;
                false
            })
            .map(|(item, _)| item)
    }

    pub fn clear(&mut self) {
        self.queue.clear();
    }

    pub fn delete_front_many(&mut self, mut remove_count: usize) {
        while let Some((_, count)) = self.queue.front_mut() {
            if *count > remove_count {
                *count -= remove_count;
                break;
            }

            remove_count -= *count;

            self.queue.pop_back();
        }
    }

    pub fn delete_back(&mut self) -> bool {
        let Some((_, count)) = self.queue.back_mut() else {
            return false;
        };

        *count -= 1;

        if *count > 0 {
            return true;
        }

        self.queue.pop_back().is_some()
    }

    pub fn delete_back_many(&mut self, mut remove_count: usize) {
        while let Some((_, count)) = self.queue.back_mut() {
            if *count > remove_count {
                *count -= remove_count;
                break;
            }

            remove_count -= *count;

            self.queue.pop_back();
        }
    }

    pub fn peek_next(&self) -> Option<&T> {
        self.queue.front().map(|(item, _)| item)
    }

    pub fn iter(&self) -> impl Iterator<Item = &T> {
        self.queue
            .iter()
            .flat_map(move |(item, count)| std::iter::repeat_n(item, *count))
    }
}

impl<T: PartialEq> RunLengthDeque<T> {
    pub fn push_back(&mut self, item: T) {
        self.push_back_many(item, 1)
    }

    pub fn push_back_many(&mut self, item: T, count: usize) {
        if let Some((stored_item, stored_count)) = self.queue.back_mut()
            && *stored_item == item
        {
            *stored_count += count;
            return;
        }

        self.queue.push_back((item, count));
    }
}

impl<T: PartialEq + Clone> RunLengthDeque<T> {
    pub fn append_clone(&mut self, other: &Self) {
        let mut other_iter = other.queue.iter();
        let Some((first_item, first_count)) = other_iter.next() else {
            return;
        };

        // merge ends
        if let Some((stored_item, stored_count)) = self.queue.back_mut()
            && stored_item == first_item
        {
            *stored_count += *first_count;
        } else {
            self.queue.push_back((first_item.clone(), *first_count));
        }

        // append the rest
        for (item, count) in other_iter {
            self.queue.push_back((item.clone(), *count));
        }
    }
}

impl<T: Clone> RunLengthDeque<T> {
    pub fn pop_front(&mut self) -> Option<T> {
        let (item, count) = self.queue.front_mut()?;

        *count -= 1;

        if *count == 0 {
            self.queue.pop_front().map(|(item, _)| item)
        } else {
            Some(item.clone())
        }
    }
}
