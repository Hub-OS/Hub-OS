pub struct LossyChunkPacker<I, C, M>
where
    I: Iterator,
    C: Fn(usize) -> usize,
    M: FnMut(&I::Item) -> usize,
{
    iterator: I,
    current_chunk: Vec<I::Item>,
    chunk_count: usize,
    calculate_chunk_limit: C,
    measure_item: M,
    chunk_size_limit: usize,
    remaining_size: usize,
}

impl<I, C, M> LossyChunkPacker<I, C, M>
where
    I: Iterator,
    C: Fn(usize) -> usize,
    M: FnMut(&I::Item) -> usize,
{
    fn flush(&mut self) -> Vec<I::Item> {
        let mut out = Vec::new();

        std::mem::swap(&mut self.current_chunk, &mut out);

        out
    }
}

impl<I, C, M> Iterator for LossyChunkPacker<I, C, M>
where
    I: Iterator,
    C: Fn(usize) -> usize,
    M: FnMut(&I::Item) -> usize,
{
    type Item = Vec<I::Item>;

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            let item = match self.iterator.next() {
                Some(item) => item,
                None => {
                    if self.current_chunk.is_empty() {
                        return None;
                    } else {
                        return Some(self.flush());
                    }
                }
            };

            let measure_item = &mut self.measure_item;
            let item_size = measure_item(&item);

            if item_size >= self.chunk_size_limit {
                // too big for any chunk, ignore
                continue;
            }

            if self.remaining_size < item_size {
                // calculate max size for the next chunk
                let calculate_chunk_limit = &self.calculate_chunk_limit;
                self.chunk_count += 1;
                self.chunk_size_limit = calculate_chunk_limit(self.chunk_count);
                self.remaining_size = self.chunk_size_limit;

                return Some(self.flush());
            }

            self.current_chunk.push(item);
            self.remaining_size -= item_size;
        }
    }
}

pub trait IteratorHelper: Iterator {
    fn pack_chunks_lossy<C, M>(
        self,
        calculate_chunk_limit: C,
        measure_item: M,
    ) -> LossyChunkPacker<Self, C, M>
    where
        C: Fn(usize) -> usize,
        M: FnMut(&Self::Item) -> usize,
        Self: Sized,
    {
        let chunk_size_limit = calculate_chunk_limit(0);

        LossyChunkPacker {
            iterator: self,
            current_chunk: Vec::new(),
            chunk_count: 0,
            calculate_chunk_limit,
            measure_item,
            chunk_size_limit,
            remaining_size: chunk_size_limit,
        }
    }
}

impl<I> IteratorHelper for I where I: Iterator {}
