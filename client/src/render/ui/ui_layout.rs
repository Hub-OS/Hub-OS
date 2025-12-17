use super::*;
use crate::bindable::GenerationalIndex;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::sync::Arc;
use std::sync::Mutex;
use taffy::node::MeasureFunc as TaffyMeasureFunc;
use taffy::prelude::Size as TaffySize;
use taffy::style::LengthPercentage;

pub use taffy::prelude::{
    AlignItems, AlignSelf, Dimension, FlexDirection, JustifyContent, LengthPercentageAuto, Position,
};

pub trait UiNode {
    fn focusable(&self) -> bool {
        false
    }

    fn is_locking_focus(&self) -> bool {
        false
    }

    fn update(&mut self, _game_io: &mut GameIO, _bounds: Rect, _focused: bool) {}

    fn draw_bounded(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, bounds: Rect);

    fn measure_ui_size(&mut self, game_io: &GameIO) -> Vec2;

    fn ui_size_dirty(&self) -> bool {
        false
    }
}

impl UiNode for () {
    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn draw_bounded(&mut self, _: &GameIO, _: &mut SpriteColorQueue, _: Rect) {}
}

pub trait IntoUiLayoutNode {
    fn into_layout_node<'a>(self) -> UiLayoutNode<'a>;
}

impl<T: UiNode + 'static> IntoUiLayoutNode for T {
    fn into_layout_node<'a>(self) -> UiLayoutNode<'a> {
        self.into()
    }
}

#[derive(Clone)]
pub struct UiStyle {
    pub flex_direction: FlexDirection,
    /// aligning children on main axis
    pub justify_content: Option<JustifyContent>,
    /// aligning children on cross axis
    pub align_items: Option<AlignItems>,
    /// align self on cross axis
    pub align_self: Option<AlignSelf>,
    pub flex_grow: f32,
    pub flex_shrink: f32,
    pub flex_basis: Dimension,
    pub position: Position,
    pub left: LengthPercentageAuto,
    pub right: LengthPercentageAuto,
    pub top: LengthPercentageAuto,
    pub bottom: LengthPercentageAuto,
    pub margin_left: LengthPercentageAuto,
    pub margin_right: LengthPercentageAuto,
    pub margin_top: LengthPercentageAuto,
    pub margin_bottom: LengthPercentageAuto,
    pub padding_left: f32,
    pub padding_right: f32,
    pub padding_top: f32,
    pub padding_bottom: f32,
    pub min_width: Dimension,
    pub min_height: Dimension,
    pub max_width: Dimension,
    pub max_height: Dimension,
    pub nine_patch: Option<NinePatch>,
    pub ime_padding: Option<f32>,
}

impl Default for UiStyle {
    fn default() -> Self {
        Self {
            flex_direction: FlexDirection::default(),
            justify_content: None,
            align_items: None,
            align_self: None,
            flex_grow: 0.0,
            flex_shrink: 1.0,
            flex_basis: Dimension::Auto,
            position: Position::default(),
            left: LengthPercentageAuto::Auto,
            right: LengthPercentageAuto::Auto,
            top: LengthPercentageAuto::Auto,
            bottom: LengthPercentageAuto::Auto,
            margin_left: LengthPercentageAuto::Points(0.0),
            margin_right: LengthPercentageAuto::Points(0.0),
            margin_top: LengthPercentageAuto::Points(0.0),
            margin_bottom: LengthPercentageAuto::Points(0.0),
            padding_left: 0.0,
            padding_right: 0.0,
            padding_top: 0.0,
            padding_bottom: 0.0,
            min_width: Dimension::Auto,
            min_height: Dimension::Auto,
            max_width: Dimension::Auto,
            max_height: Dimension::Auto,
            nine_patch: None,
            ime_padding: None,
        }
    }
}

pub struct UiLayoutNode<'a> {
    handle: Option<&'a mut Option<GenerationalIndex>>,
    content: Box<dyn UiNode>,
    style: UiStyle,
    children: Vec<UiLayoutNode<'a>>,
}

impl<'a> UiLayoutNode<'a> {
    pub fn new(content: impl UiNode + 'static) -> Self {
        Self {
            handle: None,
            content: Box::new(content),
            style: UiStyle::default(),
            children: Vec::new(),
        }
    }

    /// Same as UiLayoutNode::new(()), but self documenting
    pub fn new_container() -> Self {
        Self::new(())
    }

    pub fn with_handle(mut self, handle: &'a mut Option<GenerationalIndex>) -> Self {
        self.handle = Some(handle);
        self
    }

    pub fn with_style(mut self, style: UiStyle) -> Self {
        self.style = style;
        self
    }

    pub fn with_children(mut self, children: Vec<UiLayoutNode<'a>>) -> Self {
        self.children = children;
        self
    }
}

impl<T: UiNode + 'static> From<T> for UiLayoutNode<'_> {
    fn from(content: T) -> Self {
        Self::new(content)
    }
}

struct InternalUiElement {
    taffy_node: taffy::prelude::Node,
    taffy_size: Arc<Mutex<Vec2>>,
    content: Box<dyn UiNode>,
    nine_patch: Option<NinePatch>,
    ime_padding: Option<f32>,
    padding_left: f32,
    padding_right: f32,
    padding_top: f32,
    padding_bottom: f32,
    bounds: Rect,
}

pub struct UiLayout {
    taffy: taffy::Taffy,
    tree: Tree<InternalUiElement>,
    bounds: Rect,
    calculated: bool,
    focused: bool,
    focused_index: Option<GenerationalIndex>,
    wrap_selection: bool,
}

impl UiLayout {
    fn new(flex_direction: FlexDirection, bounds: Rect, root_nodes: Vec<UiLayoutNode>) -> Self {
        use taffy::Taffy;

        let mut taffy = Taffy::new();

        let root = to_internal_node(
            &mut taffy,
            UiLayoutNode::new_container().with_style(UiStyle {
                flex_direction,
                min_width: Dimension::Points(bounds.width),
                min_height: Dimension::Points(bounds.height),
                max_width: Dimension::Points(bounds.width),
                max_height: Dimension::Points(bounds.height),
                ..Default::default()
            }),
        );

        let mut ui_layout = Self {
            taffy,
            tree: Tree::new(root),
            bounds,
            calculated: false,
            focused: true,
            focused_index: None,
            wrap_selection: false,
        };

        ui_layout.set_children(ui_layout.tree.root_index(), root_nodes);
        ui_layout.focus_default();

        ui_layout
    }

    pub fn new_vertical(bounds: Rect, root_nodes: Vec<UiLayoutNode>) -> Self {
        Self::new(FlexDirection::Column, bounds, root_nodes)
    }

    pub fn new_horizontal(bounds: Rect, root_nodes: Vec<UiLayoutNode>) -> Self {
        Self::new(FlexDirection::Row, bounds, root_nodes)
    }

    pub fn with_style(mut self, style: UiStyle) -> Self {
        let element = self.tree.root_mut();

        self.taffy
            .set_style(element.taffy_node, derive_taffy_style(&style))
            .unwrap();

        element.nine_patch = style.nine_patch;

        self
    }

    pub fn update_style(&mut self, index: GenerationalIndex, updater: impl FnOnce(&mut UiStyle)) {
        let Some(element) = self.tree.get_mut(index) else {
            return;
        };

        let taffy_style = self.taffy.style(element.taffy_node).unwrap();

        // recreate ui style
        let mut style = UiStyle {
            // styles in taffy
            flex_direction: taffy_style.flex_direction,
            justify_content: taffy_style.justify_content,
            align_items: taffy_style.align_items,
            align_self: taffy_style.align_self,
            flex_grow: taffy_style.flex_grow,
            flex_shrink: taffy_style.flex_shrink,
            flex_basis: taffy_style.flex_basis,
            position: taffy_style.position,
            left: taffy_style.inset.left,
            right: taffy_style.inset.right,
            top: taffy_style.inset.top,
            bottom: taffy_style.inset.bottom,
            margin_left: taffy_style.margin.left,
            margin_right: taffy_style.margin.right,
            margin_top: taffy_style.margin.top,
            margin_bottom: taffy_style.margin.bottom,
            min_width: taffy_style.min_size.width,
            min_height: taffy_style.min_size.height,
            max_width: taffy_style.max_size.width,
            max_height: taffy_style.max_size.height,

            // styles on element
            nine_patch: element.nine_patch.take(),
            ime_padding: element.ime_padding,
            padding_left: element.padding_left,
            padding_right: element.padding_right,
            padding_top: element.padding_top,
            padding_bottom: element.padding_bottom,
        };

        updater(&mut style);

        // update taffy style
        let taffy_style = derive_taffy_style(&style);
        let _ = self.taffy.set_style(element.taffy_node, taffy_style);

        // update element style
        element.nine_patch = style.nine_patch;
        element.ime_padding = style.ime_padding;
        element.padding_left = style.padding_left;
        element.padding_right = style.padding_right;
        element.padding_top = style.padding_top;
        element.padding_bottom = style.padding_bottom;
    }

    pub fn with_focus(mut self, focused: bool) -> UiLayout {
        self.focused = focused;
        self
    }

    pub fn with_wrapped_selection(mut self) -> Self {
        self.wrap_selection = true;

        self
    }

    pub fn position(&self) -> Vec2 {
        self.bounds.position()
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.bounds.set_position(position);
    }

    pub fn get_child_index(
        &self,
        parent: GenerationalIndex,
        index: usize,
    ) -> Option<GenerationalIndex> {
        let parent = self.tree.get_node(parent)?;
        parent.children().get(index).copied()
    }

    pub fn set_children(&mut self, index: GenerationalIndex, mut children: Vec<UiLayoutNode>) {
        self.clear_children(index);

        let Some(element) = self.tree.get(index) else {
            return;
        };

        let taffy_node = element.taffy_node;

        children.reverse();
        let mut work_list = vec![(index, taffy_node, children)];

        while !work_list.is_empty() {
            let (parent_index, parent_taffy_node, children) = work_list.last_mut().unwrap();

            if children.is_empty() {
                work_list.pop();
                continue;
            }

            let mut child = children.pop().unwrap();

            let mut child_children = std::mem::take(&mut child.children);
            let handle = child.handle.take();
            let node = to_internal_node(&mut self.taffy, child);

            let child_taffy_node = node.taffy_node;
            self.taffy
                .add_child(*parent_taffy_node, child_taffy_node)
                .unwrap();

            let index = self.tree.insert_child(*parent_index, node).unwrap();

            if let Some(handle) = handle {
                *handle = Some(index);
            }

            child_children.reverse();
            work_list.push((index, child_taffy_node, child_children));
        }

        self.calculated = false;
    }

    pub fn clear_children(&mut self, index: GenerationalIndex) {
        // cleanup children
        self.tree.node_inherit(index, (), |node, _| {
            if node.index() == index {
                return;
            }

            let element = node.value();

            // remove node from taffy
            let _ = self.taffy.remove(element.taffy_node);

            // prevent focus on dead nodes
            if self.focused_index == Some(node.index()) {
                self.focused_index = None;
            }
        });

        // removal of children from the tree
        let Some(node) = self.tree.get_node(index) else {
            return;
        };

        for child_index in node.children().to_vec() {
            self.tree.remove(child_index);
        }

        self.calculated = false;
    }

    pub fn focused(&self) -> bool {
        self.focused
    }

    pub fn set_focused(&mut self, focused: bool) {
        self.focused = focused;
    }

    pub fn focus_default(&mut self) {
        // find the first focused element
        // using tree.nodes() as the order will be insertion order as long as elements are not removed
        for node in self.tree.nodes() {
            if node.value().content.focusable() {
                self.focused_index = Some(node.index());
                break;
            }
        }
    }

    pub fn set_focused_index(&mut self, index: Option<GenerationalIndex>) {
        let Some(index) = index else {
            self.focused_index = None;
            return;
        };

        if self.tree.get(index).is_some() {
            self.focused_index = Some(index);
        }
    }

    pub fn focused_index(&self) -> Option<GenerationalIndex> {
        self.focused_index
    }

    pub fn bounds(&self) -> Rect {
        self.get_bounds(GenerationalIndex::tree_root()).unwrap()
    }

    /// includes border + padding
    pub fn get_bounds(&self, index: GenerationalIndex) -> Option<Rect> {
        let element = self.tree.get(index)?;
        let mut bounds = element.bounds;

        if let Some(nine_patch) = &element.nine_patch {
            let left_width = nine_patch.left_width();
            let top_height = nine_patch.top_height();

            bounds.x -= left_width;
            bounds.y -= top_height;
            bounds.width += left_width + nine_patch.right_width();
            bounds.height += top_height + nine_patch.bottom_height();
        }

        bounds.x -= element.padding_left;
        bounds.y -= element.padding_top;
        bounds.width += element.padding_left + element.padding_right;
        bounds.height += element.padding_top + element.padding_bottom;

        bounds.x += self.bounds.x;
        bounds.y += self.bounds.y;

        Some(bounds)
    }

    pub fn update(&mut self, game_io: &mut GameIO, ui_input_tracker: &UiInputTracker) {
        self.recalculate_layout(game_io);

        for node in self.tree.nodes_mut() {
            let node_index = node.index();
            let element = node.value_mut();

            let focused = self.focused && Some(node_index) == self.focused_index;
            element.content.update(game_io, element.bounds, focused);

            if element.content.ui_size_dirty() {
                // self.taffy.mark_dirty(element.taffy_node).unwrap();
                self.calculated = false;
            }
        }

        if self.focused {
            let old_index = self.focused_index;

            // update focus after to make sure is_focus_locked is up to date
            self.update_focus(ui_input_tracker);

            if old_index != self.focused_index {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.cursor_move);
            }
        }
    }

    pub fn is_focus_locked(&self) -> bool {
        let focused_index = match self.focused_index {
            Some(index) => index,
            None => return false,
        };

        let focused_node = self.tree.get_node(focused_index).unwrap();
        let focused_element = focused_node.value();

        focused_element.content.is_locking_focus()
    }

    fn update_focus(&mut self, ui_input_tracker: &UiInputTracker) {
        let Some(focused_index) = self.focused_index else {
            return;
        };

        let focused_node = self.tree.get_node(focused_index).unwrap();
        let focused_element = focused_node.value();

        if focused_element.content.is_locking_focus() {
            return;
        }

        // unwrapping since the root node can't have focus
        let mut parent_node = self
            .tree
            .get_node(focused_node.parent_index().unwrap())
            .unwrap();
        let mut child_tree_index = focused_index;

        let taffy_style = self.taffy.style(parent_node.value().taffy_node).unwrap();

        // resolve input

        let cross_change = match taffy_style.flex_direction {
            FlexDirection::Row | FlexDirection::RowReverse => {
                ui_input_tracker.input_as_axis(Input::Up, Input::Down)
            }
            FlexDirection::Column | FlexDirection::ColumnReverse => {
                ui_input_tracker.input_as_axis(Input::Left, Input::Right)
            }
        };

        let change = if cross_change == 0.0 || parent_node.parent_index().is_none() {
            // normal change
            let change = match taffy_style.flex_direction {
                FlexDirection::Row | FlexDirection::RowReverse => {
                    ui_input_tracker.input_as_axis(Input::Left, Input::Right)
                }
                FlexDirection::Column | FlexDirection::ColumnReverse => {
                    ui_input_tracker.input_as_axis(Input::Up, Input::Down)
                }
            };

            if change == 0.0 {
                return;
            }

            change
        } else {
            // cross change, jump by parents
            child_tree_index = parent_node.index();
            // unwrapping as we did a check in the if
            parent_node = self
                .tree
                .get_node(parent_node.parent_index().unwrap())
                .unwrap();

            cross_change
        };

        // movement logic
        let mut parent_index = parent_node.index();
        let mut child_index = parent_node
            .children()
            .iter()
            .position(|index| *index == child_tree_index)
            .unwrap();

        child_index = self.increment_child_index(parent_node, change, child_index);

        loop {
            let parent_node = self.tree.get_node(parent_index).unwrap();

            let child_node = match parent_node.children().get(child_index) {
                Some(index) => self.tree.get_node(*index).unwrap(),
                None => {
                    // no child

                    // back up into the parent's parent
                    match parent_node.parent_index() {
                        Some(index) => {
                            let old_parent_index = parent_index;
                            parent_index = index;

                            let parent_node = self.tree.get_node(parent_index).unwrap();
                            child_index = parent_node
                                .children()
                                .iter()
                                .position(|index| *index == old_parent_index)
                                .unwrap();

                            child_index =
                                self.increment_child_index(parent_node, change, child_index);

                            continue;
                        }
                        None => {
                            if !self.wrap_selection {
                                // no more elements
                                // todo: maybe an out of elements event?
                                return;
                            }

                            if change < 0.0 {
                                // wrap back to the last node
                                let last_index = self.last_element_index();
                                let last_node = self.tree.get_node(last_index).unwrap();

                                parent_index = last_node.parent_index().unwrap();
                                let parent_node = self.tree.get_node(parent_index).unwrap();
                                child_index = parent_node.children().len() - 1;
                            } else {
                                // wrap back to the first node
                                parent_index = self.tree.root_index();
                                child_index = 0;
                            }

                            continue;
                        }
                    }
                }
            };

            if child_node.value().content.focusable() {
                self.focused_index = Some(child_node.index());
                return;
            }

            if child_node.children().is_empty() {
                // switch to a sibling
                child_index = self.increment_child_index(parent_node, change, child_index);
            } else {
                // this node has children, enter it
                parent_index = child_node.index();
                child_index = if change < 0.0 {
                    // enter from the end since we're backing into it
                    child_node.children().len() - 1
                } else {
                    // enter from the start
                    0
                };
            }
        }
    }

    fn last_element_index(&self) -> GenerationalIndex {
        let mut node = self.tree.root_node();

        loop {
            if node.children().is_empty() {
                return node.index();
            }

            let last_index = *node.children().last().unwrap();
            node = self.tree.get_node(last_index).unwrap();
        }
    }

    // increments based on flex direction and change direction
    fn increment_child_index(
        &self,
        parent_node: &TreeNode<InternalUiElement>,
        change: f32,
        index: usize,
    ) -> usize {
        let taffy_style = self.taffy.style(parent_node.value().taffy_node).unwrap();

        let reversed = matches!(
            taffy_style.flex_direction,
            FlexDirection::RowReverse | FlexDirection::ColumnReverse
        );

        if reversed ^ (change < 0.0) {
            index.wrapping_sub(1)
        } else {
            index.wrapping_add(1)
        }
    }

    pub fn recalculate_layout(&mut self, game_io: &GameIO) {
        if self.calculated {
            return;
        }

        for value in self.tree.values_mut() {
            let size = value.content.measure_ui_size(game_io);

            if let Ok(mut stored) = value.taffy_size.lock() {
                *stored = size;
            }
        }

        let taffy_root = self.tree.root().taffy_node;
        self.taffy
            .compute_layout(
                taffy_root,
                TaffySize {
                    width: taffy::style::AvailableSpace::MaxContent,
                    height: taffy::style::AvailableSpace::MaxContent,
                },
            )
            .unwrap();

        self.tree.inherit(
            self.tree.root_index(),
            Vec2::ZERO,
            |element, parent_position| {
                let layout = self.taffy.layout(element.taffy_node).unwrap();
                let mut internal_bounds = Rect::new(
                    layout.location.x + parent_position.x,
                    layout.location.y + parent_position.y,
                    layout.size.width,
                    layout.size.height,
                );
                let position = internal_bounds.position();

                if let Some(nine_patch) = &mut element.nine_patch {
                    let left_width = nine_patch.left_width();
                    internal_bounds.x += left_width;
                    internal_bounds.width -= left_width + nine_patch.right_width();

                    let top_height = nine_patch.top_height();
                    internal_bounds.y += top_height;
                    internal_bounds.height -= top_height + nine_patch.bottom_height();
                }

                internal_bounds.x += element.padding_left;
                internal_bounds.y += element.padding_top;
                internal_bounds.width -= element.padding_left + element.padding_right;
                internal_bounds.height -= element.padding_top + element.padding_bottom;

                element.bounds = internal_bounds;

                position
            },
        );

        self.calculated = true;
    }

    pub fn corrected_ime_height(game_io: &GameIO) -> f32 {
        let window = game_io.window();
        let view_render_offset = window.render_offset().y;
        let scale = RESOLUTION_F.y / (window.size().y as f32 - view_render_offset * 2.0);
        let height = (window.ime_height() as f32 - view_render_offset) * scale;
        height.max(0.0)
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.recalculate_layout(game_io);

        let mut offset_y = self.bounds.y;

        if game_io.input().accepting_text()
            && let Some(index) = self.focused_index
        {
            let element = &self.tree[index];

            if let Some(padding) = element.ime_padding {
                let mut bounds = element.bounds;
                bounds.y += offset_y;

                // try to move the bounds into the available space
                let ime_height = Self::corrected_ime_height(game_io);
                let available_space = RESOLUTION_F.y - ime_height - padding;

                if bounds.bottom() > available_space {
                    offset_y += available_space - bounds.bottom();
                }
            }
        }

        for element in self.tree.values_mut() {
            let mut bounds = element.bounds;
            bounds.x += self.bounds.x;
            bounds.y += offset_y;

            if let Some(nine_patch) = &mut element.nine_patch {
                let mut patch_bounds = bounds;
                patch_bounds.x -= element.padding_left;
                patch_bounds.y -= element.padding_top;
                patch_bounds.width += element.padding_left + element.padding_right;
                patch_bounds.height += element.padding_top + element.padding_bottom;

                nine_patch.draw(sprite_queue, patch_bounds);
            }

            element.content.draw_bounded(game_io, sprite_queue, bounds);
        }
    }
}

fn to_internal_node(taffy: &mut taffy::Taffy, node: UiLayoutNode) -> InternalUiElement {
    let taffy_size = Arc::new(Mutex::new(Vec2::ZERO));

    let taffy_node = taffy
        .new_leaf_with_measure(
            derive_taffy_style(&node.style),
            create_taffy_measure_fn(taffy_size.clone(), &node.style),
        )
        .unwrap();

    InternalUiElement {
        taffy_node,
        taffy_size,
        content: node.content,
        nine_patch: node.style.nine_patch,
        ime_padding: node.style.ime_padding,
        padding_left: node.style.padding_left,
        padding_right: node.style.padding_right,
        padding_top: node.style.padding_top,
        padding_bottom: node.style.padding_bottom,
        bounds: Rect::ZERO,
    }
}

fn create_taffy_measure_fn(size_cell: Arc<Mutex<Vec2>>, style: &UiStyle) -> TaffyMeasureFunc {
    let mut size = (style.nine_patch.as_ref())
        .map(|nine_patch| {
            Vec2::new(
                nine_patch.left_width() + nine_patch.right_width(),
                nine_patch.top_height() + nine_patch.bottom_height(),
            )
        })
        .unwrap_or_default();

    size.x += style.padding_left + style.padding_right;
    size.y += style.padding_top + style.padding_bottom;

    TaffyMeasureFunc::Boxed(Box::new(move |incoming_size, _available_space| {
        let calculated_size = size_cell.lock().as_deref().cloned().unwrap_or_default() + size;

        let width = match incoming_size.width {
            Some(width) => width.max(calculated_size.x),
            None => calculated_size.x,
        };

        let height = match incoming_size.height {
            Some(height) => height.max(calculated_size.y),
            None => calculated_size.y,
        };

        TaffySize { width, height }
    }))
}

fn derive_taffy_style(style: &UiStyle) -> taffy::prelude::Style {
    use taffy::prelude::Rect as TaffyRect;

    let border = (style.nine_patch.as_ref())
        .map(|nine_patch| TaffyRect {
            left: LengthPercentage::Points(nine_patch.left_width()),
            right: LengthPercentage::Points(nine_patch.right_width()),
            top: LengthPercentage::Points(nine_patch.top_height()),
            bottom: LengthPercentage::Points(nine_patch.bottom_height()),
        })
        .unwrap_or(TaffyRect::zero());

    taffy::prelude::Style {
        flex_direction: style.flex_direction,
        justify_content: style.justify_content,
        align_items: style.align_items,

        align_self: style.align_self,
        flex_grow: style.flex_grow,
        flex_shrink: style.flex_shrink,
        flex_basis: style.flex_basis,

        position: style.position,
        inset: TaffyRect {
            left: style.left,
            right: style.right,
            top: style.top,
            bottom: style.bottom,
        },

        margin: TaffyRect {
            left: style.margin_left,
            right: style.margin_right,
            top: style.margin_top,
            bottom: style.margin_bottom,
        },

        padding: TaffyRect {
            left: LengthPercentage::Points(style.padding_left),
            right: LengthPercentage::Points(style.padding_right),
            top: LengthPercentage::Points(style.padding_top),
            bottom: LengthPercentage::Points(style.padding_bottom),
        },

        border,

        min_size: TaffySize {
            width: style.min_width,
            height: style.min_height,
        },

        max_size: TaffySize {
            width: style.max_width,
            height: style.max_height,
        },

        ..Default::default()
    }
}
