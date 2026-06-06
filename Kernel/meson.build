project('wyvern', 'c',
  version: '0.1.0',
  meson_version: '>=0.56',
  default_options: [
    'warning_level=2',
    'b_staticpic=false',
    'b_pie=false',
    ''
  ]
)

hal = get_option('hal')
cpu = target_machine.cpu_family()

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
  # '-T', meson.current_source_dir() / 'HAL' / hal / 'linker.ld',
]

kernel_inc = [
  include_directories('.'),
  include_directories('Core'),
  include_directories('HAL' / hal),
]

ss = import('sourceset')
kernel_ss = ss.source_set()

subdir('Core')
subdir('HAL' / hal)

kernel_result = kernel_ss.apply({})

executable('kernel',
  sources: kernel_result.sources(),
  include_directories: kernel_inc,
  c_args: kernel_c_args,
  link_args: kernel_link_args,
)
