// might be worth switching to vec-tree

use crate::bindable::GenerationalIndex;

pub type TreeIndex = GenerationalIndex;

pub struct Node<T> {
    index: TreeIndex,
    parent: Option<TreeIndex>,
    children: Vec<TreeIndex>,
    value: T,
}

impl<T> Node<T> {
    pub fn index(&self) -> TreeIndex {
        self.index
    }

    pub fn parent_index(&self) -> Option<TreeIndex> {
        self.parent
    }

    pub fn children(&self) -> &[TreeIndex] {
        &self.children
    }

    pub fn value(&self) -> &T {
        &self.value
    }

    pub fn value_mut(&mut self) -> &mut T {
        &mut self.value
    }
}

impl<T: Clone> Clone for Node<T> {
    fn clone(&self) -> Self {
        Self {
            index: self.index,
            parent: self.parent,
            children: self.children.clone(),
            value: self.value.clone(),
        }
    }
}

pub struct Tree<T> {
    len: usize,
    nodes: Vec<(usize, Option<Node<T>>)>,
}

impl<T> Tree<T> {
    pub fn new(root_value: T) -> Self {
        Self {
            len: 1,
            nodes: vec![(
                0,
                Some(Node {
                    index: TreeIndex::new(0, 0),
                    parent: None,
                    children: Vec::new(),
                    value: root_value,
                }),
            )],
        }
    }

    pub fn root_index(&self) -> TreeIndex {
        TreeIndex {
            index: 0,
            generation: 0,
        }
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn insert_root_child(&mut self, value: T) -> TreeIndex {
        self.insert_child(self.root_index(), value).unwrap()
    }

    pub fn insert_child(&mut self, parent: TreeIndex, value: T) -> Option<TreeIndex> {
        if self.nodes.len() <= parent.index {
            return None;
        }

        let child_index = match self.nodes.iter().position(|(_, slot)| slot.is_none()) {
            Some(index) => index,
            None => {
                self.nodes.push((0, None));
                self.nodes.len() - 1
            }
        };

        let tree_index = TreeIndex {
            index: child_index,
            generation: self.nodes[child_index].0,
        };

        if let Some(parent_node) = &mut self.nodes[parent.index].1 {
            if parent_node.index.generation != parent.generation {
                return None;
            }
            parent_node.children.push(tree_index);
        } else {
            return None;
        };

        self.len += 1;
        self.nodes[child_index].1 = Some(Node {
            index: tree_index,
            parent: Some(parent),
            children: Vec::new(),
            value,
        });

        Some(tree_index)
    }

    pub fn root(&self) -> &T {
        &self.root_node().value
    }

    pub fn root_mut(&mut self) -> &mut T {
        &mut self.root_node_mut().value
    }

    pub fn root_node(&self) -> &Node<T> {
        self.nodes[0].1.as_ref().unwrap()
    }

    pub fn root_node_mut(&mut self) -> &mut Node<T> {
        self.nodes[0].1.as_mut().unwrap()
    }

    pub fn get(&self, index: TreeIndex) -> Option<&T> {
        let node = self.nodes.get(index.index)?.1.as_ref()?;

        if node.index.generation != index.generation {
            return None;
        }

        Some(&node.value)
    }

    pub fn get_mut(&mut self, index: TreeIndex) -> Option<&mut T> {
        let node = self.nodes.get_mut(index.index)?.1.as_mut()?;

        if node.index.generation != index.generation {
            return None;
        }

        Some(&mut node.value)
    }

    pub fn get_node(&self, index: TreeIndex) -> Option<&Node<T>> {
        self.nodes
            .get(index.index)?
            .1
            .as_ref()
            .filter(|node| node.index == index)
    }

    pub fn get_node_mut(&mut self, index: TreeIndex) -> Option<&mut Node<T>> {
        self.nodes
            .get_mut(index.index)?
            .1
            .as_mut()
            .filter(|node| node.index == index)
    }

    pub fn remove(&mut self, index: TreeIndex) -> Option<T> {
        // validate index
        self.get(index)?;

        let TreeIndex { index, .. } = index;

        if index == 0 {
            // prevent removal of the root
            return None;
        }

        let removed_node = self.nodes.get_mut(index)?.1.take()?;

        if let Some(parent) = &mut self.nodes[removed_node.parent.unwrap().index].1 {
            // remove node from the parent list
            let child_index = parent
                .children
                .iter()
                .position(|i| i.index == index)
                .unwrap();

            parent.children.remove(child_index);
        }

        // dropping children from the tree
        // removing reversed to prevent shifting
        for index in removed_node.children.iter().rev() {
            self.remove(*index);
        }

        // reduce len
        self.len -= 1;

        // increase generation
        self.nodes[index].0 += 1;

        Some(removed_node.value)
    }

    /// Passes a value generated from parent to children
    pub fn inherit<F, V>(
        &mut self,
        start_index: GenerationalIndex,
        initial_value: V,
        mut callback: F,
    ) where
        F: FnMut(&mut T, &V) -> V,
    {
        let start_value = match self.get_mut(start_index) {
            Some(value) => value,
            None => return,
        };

        let root_value = callback(start_value, &initial_value);

        let mut stack: Vec<(V, usize, usize)> = vec![(root_value, start_index.index, 0)];

        while let Some((value, index, child_index)) = stack.last() {
            let node = self.nodes[*index].1.as_mut().unwrap();

            if let Some(index) = node.children.get(*child_index).cloned() {
                let child = self.nodes[index.index].1.as_mut().unwrap();
                let child_value = callback(&mut child.value, value);
                stack.push((child_value, index.index, 0));
            } else {
                stack.pop();

                if let Some((_, _, child_index)) = stack.last_mut() {
                    *child_index += 1;
                }
            }
        }
    }

    /// Passes a value generated from parent to children
    pub fn node_inherit<F, V>(
        &mut self,
        start_index: GenerationalIndex,
        initial_value: V,
        mut callback: F,
    ) where
        F: FnMut(&mut Node<T>, &V) -> V,
    {
        let start_node = match self.get_node_mut(start_index) {
            Some(node) => node,
            None => return,
        };

        let root_value = callback(start_node, &initial_value);

        let mut stack: Vec<(V, usize, usize)> = vec![(root_value, start_index.index, 0)];

        while let Some((value, index, child_index)) = stack.last() {
            let node = self.nodes[*index].1.as_mut().unwrap();

            if let Some(index) = node.children.get(*child_index).cloned() {
                let child = self.nodes[index.index].1.as_mut().unwrap();
                let child_value = callback(child, value);
                stack.push((child_value, index.index, 0));
            } else {
                stack.pop();

                if let Some((_, _, child_index)) = stack.last_mut() {
                    *child_index += 1;
                }
            }
        }
    }

    // Visits all nodes in arbitrary order
    pub fn nodes(&self) -> impl Iterator<Item = &Node<T>> {
        self.nodes.iter().filter_map(|(_, node)| node.as_ref())
    }

    // Visits all nodes in arbitrary order
    pub fn nodes_mut(&mut self) -> impl Iterator<Item = &mut Node<T>> {
        self.nodes.iter_mut().filter_map(|(_, node)| node.as_mut())
    }

    // Visits all values in arbitrary order
    pub fn values(&self) -> impl Iterator<Item = &T> {
        self.nodes
            .iter()
            .filter_map(|(_, node)| node.as_ref())
            .map(|node| &node.value)
    }

    // Visits all values mutably in arbitrary order
    pub fn values_mut(&mut self) -> impl Iterator<Item = &mut T> {
        self.nodes
            .iter_mut()
            .filter_map(|(_, node)| node.as_mut())
            .map(|node| &mut node.value)
    }
}

impl<T: Clone> Clone for Tree<T> {
    fn clone(&self) -> Self {
        Self {
            len: self.len,
            nodes: self.nodes.clone(),
        }
    }
}

// maybe this should be returning nodes and not values?
impl<T> std::ops::Index<TreeIndex> for Tree<T> {
    type Output = T;

    fn index(&self, index: TreeIndex) -> &Self::Output {
        let node = self.nodes[index.index].1.as_ref().unwrap();

        assert_eq!(node.index, index);

        &node.value
    }
}

impl<T> std::ops::IndexMut<TreeIndex> for Tree<T> {
    fn index_mut(&mut self, index: TreeIndex) -> &mut Self::Output {
        let node = self.nodes[index.index].1.as_mut().unwrap();

        assert_eq!(node.index, index);

        &mut node.value
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // out of bounds index
    // possible to receive invalidly using an index from another tree
    const OOB_INDEX: TreeIndex = TreeIndex {
        index: 1000,
        generation: 0,
    };

    #[test]
    fn insertion_and_removal() {
        let mut tree: Tree<i32> = Tree::new(0);

        assert_eq!(
            tree.insert_child(OOB_INDEX, 1),
            None,
            "out of bounds creation"
        );

        let indices = [
            TreeIndex::new(0, 0),
            tree.insert_child(TreeIndex::new(0, 0), 1).unwrap(), // 1
            tree.insert_child(TreeIndex::new(0, 0), 2).unwrap(), // 2
            tree.insert_child(TreeIndex::new(0, 0), 3).unwrap(), // 3
            tree.insert_child(TreeIndex::new(2, 0), 4).unwrap(), // 4
            tree.insert_child(TreeIndex::new(2, 0), 5).unwrap(), // 5
            tree.insert_child(TreeIndex::new(2, 0), 6).unwrap(), // 6
        ];

        assert_eq!(
            tree.remove(TreeIndex::new(0, 0)),
            None,
            "can't destroy root"
        );

        assert_eq!(
            tree.insert_child(OOB_INDEX, 1),
            None,
            "out of bounds removal"
        );
        assert_eq!(tree.remove(indices[2]), Some(2), "in bounds removal");

        assert_eq!(tree.remove(indices[4]), None, "child should be removed");
        assert_eq!(tree.get(indices[5]), None, "child should be removed");
        assert_eq!(tree.get(indices[6]), None, "child should be removed");

        assert_eq!(
            tree.root_node().children,
            vec![TreeIndex::new(1, 0), TreeIndex::new(3, 0)]
        );
    }

    #[test]
    fn inherit() {
        // value represents depth

        let mut tree: Tree<i32> = Tree::new(0);

        tree.insert_child(TreeIndex::new(0, 0), 1); // 1
        tree.insert_child(TreeIndex::new(0, 0), 1); // 2
        tree.insert_child(TreeIndex::new(2, 0), 2); // 3
        tree.insert_child(TreeIndex::new(1, 0), 2); // 4
        tree.insert_child(TreeIndex::new(0, 0), 1); // 5
        tree.insert_child(TreeIndex::new(3, 0), 3); // 6
        tree.insert_child(TreeIndex::new(0, 0), 1); // 7
        tree.insert_child(TreeIndex::new(6, 0), 4); // 6

        tree.inherit(tree.root_index(), 0, |i, v| {
            // testing depth
            assert_eq!(*v, *i);

            // tracking depth
            v + 1
        });
    }

    #[test]
    fn generations() {
        let mut tree: Tree<i32> = Tree::new(0);

        let index = tree.insert_child(TreeIndex::new(0, 0), 1).unwrap();
        assert_eq!(tree.get(index).cloned(), Some(1));

        tree.remove(index);

        let index2 = tree.insert_child(TreeIndex::new(0, 0), 1).unwrap();

        assert_eq!(index.index, index2.index);
        assert_eq!(tree.get(index), None);
    }
}
