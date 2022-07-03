use opencl3::{
    command_queue::CommandQueue,
    context::Context,
    device::{get_all_devices, Device, CL_DEVICE_TYPE_GPU},
    error_codes::ClError,
    kernel::Kernel,
    program::{Program, CL_BUILD_ERROR},
};

pub struct ClMetaContext {
    /// this is in here to control the drop point in time
    _device: Device,
    /// this is in here to control the drop point in time
    pub context: Context,
    pub queue: CommandQueue,
    /// this is in here to control the drop point in time
    _program: Program,
    pub kernel: Kernel,
}

#[derive(thiserror::Error, Debug)]
pub enum CreationError {
    #[error("{:?}: {}", .0, <&str>::from(ClError((.0).0)))]
    OpenCl(ClError),
    #[error("OpenCL Program Build error: {0}")]
    ClBuild(String),
    #[error("No GPU found at {0}")]
    NoGpu(usize),
}

impl From<ClError> for CreationError {
    fn from(err: ClError) -> Self {
        Self::OpenCl(err)
    }
}

impl ClMetaContext {
    pub fn full_init_with_build(
        gpu_idx: usize,
        source: &str,
        kernel_name: &str,
    ) -> Result<Self, CreationError> {
        let device = Device::from(
            *get_all_devices(CL_DEVICE_TYPE_GPU)?
                .get(gpu_idx)
                .ok_or(CreationError::NoGpu(gpu_idx))?,
        );

        let context = Context::from_device(&device)?;

        let queue = CommandQueue::create_with_properties(&context, device.id(), 0, 0)?;

        let mut program = Program::create_from_source(&context, source)?;

        program.build(context.devices(), "").map_err(|err| {
            if matches!(err, ClError(CL_BUILD_ERROR)) {
                CreationError::ClBuild(
                    program
                        .get_build_log(context.devices()[0])
                        .expect("only one device should be in context"),
                )
            } else {
                err.into()
            }
        })?;

        let kernel = Kernel::create(&program, kernel_name)?;

        Ok(Self {
            context,
            _device: device,
            kernel,
            _program: program,
            queue,
        })
    }
}
