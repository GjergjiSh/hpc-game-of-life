use std::env::Args as EnvArgs;

#[derive(thiserror::Error, Debug)]
pub enum ArgsError {
    #[error("Not enough arguments")]
    ArgNum(),
    #[error("Could not parse {argfield}")]
    ParseInt {
        argfield: &'static str,
        #[source]
        source: std::num::ParseIntError,
    },
    #[error("Could not parse {argfield}")]
    ParseBool {
        argfield: &'static str,
        #[source]
        source: std::str::ParseBoolError,
    },
}

#[derive(Clone, Copy, Debug)]
pub struct Args {
    pub row_number: usize,
    pub column_number: usize,
    pub generations: usize,
    pub display: bool,
}

impl TryFrom<EnvArgs> for Args {
    type Error = ArgsError;

    fn try_from(mut value: EnvArgs) -> Result<Self, Self::Error> {
        // skip program name
        value.next();

        use ArgsError::*;

        Ok(Self {
            row_number: value
                .next()
                .ok_or(ArgsError::ArgNum())?
                .parse()
                .map_err(|source| ParseInt {
                    argfield: "row_number",
                    source,
                })?,
            column_number: value
                .next()
                .ok_or(ArgsError::ArgNum())?
                .parse()
                .map_err(|source| ParseInt {
                    argfield: "column_number",
                    source,
                })?,
            generations: value
                .next()
                .ok_or(ArgsError::ArgNum())?
                .parse()
                .map_err(|source| ParseInt {
                    argfield: "generations",
                    source,
                })?,
            display: value
                .next()
                .ok_or(ArgsError::ArgNum())?
                .parse()
                .map_err(|source| ParseBool {
                    argfield: "display",
                    source,
                })?,
        })
    }
}
