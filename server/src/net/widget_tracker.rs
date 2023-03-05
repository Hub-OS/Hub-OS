use std::collections::VecDeque;

pub struct WidgetTracker<T> {
    textbox_queue: VecDeque<T>,
    bbs_queue: VecDeque<T>,
    shop_queue: VecDeque<T>,
    active_bbs: Option<T>,
    active_shop: Option<T>,
}

impl<T> WidgetTracker<T> {
    pub fn new() -> WidgetTracker<T> {
        WidgetTracker {
            textbox_queue: VecDeque::new(),
            bbs_queue: VecDeque::new(),
            shop_queue: VecDeque::new(),
            active_bbs: None,
            active_shop: None,
        }
    }

    pub fn is_empty(&self) -> bool {
        self.textbox_queue.is_empty()
            && self.active_bbs.is_none()
            && self.bbs_queue.is_empty()
            && self.active_shop.is_none()
    }

    pub fn track_textbox(&mut self, owner: T) {
        self.textbox_queue.push_back(owner);
    }

    pub fn pop_textbox(&mut self) -> Option<T> {
        self.textbox_queue.pop_front()
    }

    pub fn track_board(&mut self, owner: T) {
        self.bbs_queue.push_back(owner);
    }

    pub fn open_board(&mut self) {
        if let Some(owner) = self.bbs_queue.pop_front() {
            self.active_bbs = Some(owner)
        }
    }

    pub fn current_board(&mut self) -> Option<&T> {
        self.active_bbs.as_ref()
    }

    pub fn close_board(&mut self) -> Option<T> {
        self.active_bbs.take()
    }

    pub fn track_shop(&mut self, owner: T) {
        self.shop_queue.push_back(owner);
    }

    pub fn open_shop(&mut self) {
        if let Some(owner) = self.shop_queue.pop_front() {
            self.active_shop = Some(owner)
        }
    }

    pub fn current_shop(&self) -> Option<&T> {
        self.active_shop.as_ref()
    }

    pub fn close_shop(&mut self) -> Option<T> {
        self.active_shop.take()
    }
}
