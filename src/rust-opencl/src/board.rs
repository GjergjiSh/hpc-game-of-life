use std::fmt::Display;

pub trait FieldTypeBounds: Display + From<bool> {}

impl<T> FieldTypeBounds for T where T: Display, T: From<bool> {}

pub struct Board<FieldType: FieldTypeBounds = u32> {
    pub column_number: usize,
    pub row_number: usize,
    pub entries: Vec<FieldType>,
}

impl<T: FieldTypeBounds> std::fmt::Display for Board<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for row in 0..self.row_number {
            for column in 0..self.column_number {
                write!(f, "{}", self.entries[row * self.column_number + column])?;
            }
            write!(f, "\n")?;
        };
        write!(f, "\n")?;
        Ok(())
    }
}

impl<T: FieldTypeBounds> Board<T> {
    pub fn total_size(&self) -> usize {
        self.column_number * self.row_number
    }

    pub fn get_at<'a>(&'a mut self, x: usize, y: usize) -> Option<&'a mut T> {
        self.entries.get_mut(y * self.column_number + x)
    }

    fn new_empty(column_number: usize, row_number: usize) -> Self
        where T: Clone
    {
        Self {
            row_number, column_number, entries: vec![false.into(); row_number * column_number]
        }
    }

    pub fn new_glider(column_number: usize, row_number: usize) -> anyhow::Result<Self>
        where T: Clone
    {
        assert!(column_number > 3 && row_number > 3, "Bord unsufficient large for glider");
        let mut empty = Self::new_empty(column_number, row_number);
        for (x, y) in [
            (1, 2),
            (2, 3),
            (3, 1),
            (3, 2),
            (3, 3),
        ] {
            *empty.get_at(x, y).ok_or(anyhow::format_err!("Field is not large enough"))? = true.into()
        };
        Ok(empty)
    }
}