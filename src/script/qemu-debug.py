import os

# 定义磁盘文件名称
disk1 = "disk1.vhd"
disk2 = "disk2.vhd"

# 生成 QEMU 命令参数列表
qemu_args = [
    "start", "qemu-system-i386",
    "-m", "128M",
    "-s",
    "-S",
    "-serial", "stdio",
    "-drive", f"file={disk1},index=0,media=disk,format=raw",
    "-drive", f"file={disk2},index=1,media=disk,format=raw",
    "-d", "pcall,page,mmu,cpu_reset,guest_errors,page,trace:ps2_keyboard_set_translation"
]

# 将列表转换为命令字符串并执行
qemu_command = " ".join(qemu_args)
os.system(qemu_command)