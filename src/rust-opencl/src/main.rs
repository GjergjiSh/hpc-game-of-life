const GOL_OPENCL_SOURCE: &'static str = include_str!("gol_work_group.cl");
// has to be the name of the choosen kernel inside the cl file
const GOL_KERNEL_NAME: &'static str = "game_of_life_split";

const BLOCK_SIZE: usize = 6;
const MEM_BLOCK_SIZE: usize = BLOCK_SIZE + 2;

mod args;
mod opencl_meta_ctx;
mod board;

use opencl3::{
    error_codes::ClError,
    kernel::ExecuteKernel,
    memory::{Buffer, CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY, CL_MEM_READ_WRITE},
    types::{cl_bool, cl_int, CL_BLOCKING, CL_NON_BLOCKING},
};
use std::{env::args, time::Instant};

use opencl_meta_ctx::{ClMetaContext};

#[derive(thiserror::Error, Debug)]
enum OpenClStdErr {
    #[error("OpenCL Error #{}: {:?}", (.0).0, 0)]
    OpenCl(ClError),
}

impl From<ClError> for OpenClStdErr {
    fn from(err: ClError) -> Self {
        Self::OpenCl(err)
    }
}

use args::Args;
use board::Board;
use anyhow::{Result, Context};

fn main() -> Result<()> {
    let args = Args::try_from(args()).context("Failed to parse arguments")?;

    let cl_meta_ctx = ClMetaContext::full_init_with_build(0, GOL_OPENCL_SOURCE, GOL_KERNEL_NAME)
        .context("Failed to initialize OpenCL")?;

    let mut board = Board::new_glider(
        args.column_number, 
        args.row_number,
    )?;

    let create_buffer = || {
        Buffer::<cl_bool>::create(
            &cl_meta_ctx.context,
            CL_MEM_READ_WRITE,
            board.total_size(),
            std::ptr::null_mut(),
        )
            .map_err(OpenClStdErr::from)
            .context("Failed to create OpenCL Buffer")
    };

    let mut buf1 = create_buffer()?;

    let mut buf2 = create_buffer()?;

    cl_meta_ctx.queue.enqueue_write_buffer(
        &mut buf1, 
        CL_BLOCKING, 
        0, 
        board.entries.as_slice(), 
        &[],
    ).map_err(OpenClStdErr::from)?;
    if args.display {
        println!("generation 0:\n{board}");
    }

    let start = Instant::now();

    for gen in 0..args.generations {
        let (src_field, dst_field) = if gen % 2 == 0 {
            (&mut buf1, &mut buf2)
        } else {
            (&mut buf2, &mut buf1)
        };
        ExecuteKernel::new(&cl_meta_ctx.kernel)
            .set_arg(&board.column_number)
            .set_arg(&board.row_number)
            .set_arg(src_field)
            .set_arg(dst_field)
            .set_arg_local_buffer(std::mem::size_of::<cl_bool>() * MEM_BLOCK_SIZE * MEM_BLOCK_SIZE)
            .set_local_work_sizes(&[MEM_BLOCK_SIZE,  MEM_BLOCK_SIZE])
            .set_global_work_sizes(&[
                board.row_number + (board.row_number / BLOCK_SIZE) * 2,
                board.column_number + (board.column_number / BLOCK_SIZE) * 2
            ])
            // SHOULD be unneccessary?
            // .set_wait_event(event)
            .enqueue_nd_range(&cl_meta_ctx.queue).map_err(OpenClStdErr::from)?;
        if args.display {
            cl_meta_ctx.queue.enqueue_read_buffer(
                dst_field,
                CL_BLOCKING,
                0,
                board.entries.as_mut_slice(),
                &[],
            ).map_err(OpenClStdErr::from)?;
            println!("generation {gen}:\n{board}");
        }
    }

    let run_time = start.elapsed();

    println!("Execution with Rusts OpenCL wrapper took {sec_float} sec", sec_float = run_time.as_secs_f32());

    Ok(())
}
