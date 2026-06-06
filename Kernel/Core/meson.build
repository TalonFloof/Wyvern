kernel_ss.add(files('main.c'))

if target_machine.cpu_family() == 'riscv64'
  kernel_ss.add(files('Arch/RiscV64/arch.c'))
elif target_machine.cpu_family() == 'aarch64'
  kernel_ss.add(files('Arch/AArch64/arch.c'))
elif target_machine.cpu_family() == 'x86_64'
  kernel_ss.add(files('Arch/x86_64/arch.c'))
endif
