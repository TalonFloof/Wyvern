project('wyvern', 'c',
  version: '0.1.0',
  meson_version: '>=0.56',
  default_options: [
    'warning_level=2',
    'b_staticpic=false',
    'b_pie=false',
  ]
)

platform = get_option('platform')
cpu = target_machine.cpu_family()

# Derive the arch-level HAL directory from the cross file's cpu_family.
# Add a branch here when porting to a new architecture.
if cpu == 'riscv64'
  hal = 'RiscV64'
elif cpu == 'aarch64'
  hal = 'AArch64'
elif cpu == 'x86_64'
  hal = 'x86_64'
else
  error('Unsupported cpu_family: ' + cpu)
endif

# Validate that the chosen platform belongs to this architecture.
riscv64_platforms = ['QEMU', 'VisionFive2']
aarch64_platforms = ['Broadcom2837']
x86_64_platforms  = ['NewWorldPC']

if cpu == 'riscv64'
  assert(platform in riscv64_platforms,
    '"@0@" is not a RiscV64 platform. Valid: @1@'.format(platform, ', '.join(riscv64_platforms)))
elif cpu == 'aarch64'
  assert(platform in aarch64_platforms,
    '"@0@" is not an AArch64 platform. Valid: @1@'.format(platform, ', '.join(aarch64_platforms)))
elif cpu == 'x86_64'
  assert(platform in x86_64_platforms,
    '"@0@" is not an x86_64 platform. Valid: @1@'.format(platform, ', '.join(x86_64_platforms)))
endif

kernel_c_args = [
  '-ffreestanding',
  '-fno-stack-protector',
  '-fno-builtin',
  '-Wall',
  '-Wextra',
]

if cpu == 'riscv64'
  kernel_c_args += [
    '-march=rv64imac',
    '-mabi=lp64',
    '-mcmodel=medany',
  ]
elif cpu == 'aarch64'
  kernel_c_args += ['-mgeneral-regs-only']
elif cpu == 'x86_64'
  kernel_c_args += [
    '-m64',
    '-mno-red-zone',
    '-mcmodel=kernel',
    '-mno-mmx',
    '-mno-sse',
    '-mno-sse2',
  ]
endif

kernel_link_args = [
  '-nostdlib',
  '-nostartfiles',
  '-static',
  # '-T', meson.current_source_dir() / 'Core/HAL' / hal / 'Platform' / platform / 'linker.ld',
]

kernel_inc = [
  include_directories('.'),
  include_directories('Core'),
  include_directories('Core/HAL'),                                   # hal.h
  include_directories('Core/HAL' / hal),                            # arch-level headers
  include_directories('Core/HAL' / hal / 'Platform' / platform),   # platform headers
]

ss = import('sourceset')
kernel_ss = ss.source_set()

subdir('Core')                                           # main.c
subdir('Core/HAL' / hal)                               # arch-level: shared across all boards on this arch
subdir('Core/HAL' / hal / 'Platform' / platform)      # platform-level: board-specific code

kernel_result = kernel_ss.apply({})

executable('kernel',
  sources: kernel_result.sources(),
  include_directories: kernel_inc,
  c_args: kernel_c_args,
  link_args: kernel_link_args,
)
