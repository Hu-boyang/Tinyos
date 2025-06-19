import os
import platform

# 检查是否为Windows系统
if platform.system() != "Windows":
    print("此脚本仅支持Windows系统。")
    exit(1)

# 定义要检测的文件路径
file_name1 = "disk1.vhd"
file_name2 = "disk2.vhd"

# 检查文件是否存在
if not os.path.exists(file_name1):
    print(f"文件 {file_name1} 不存在。")
    exit(1)

if not os.path.exists(file_name2):
    print(f"文件 {file_name2} 不存在。")
    exit(1)

# 写入 disk1.vhd
os.system(f"dd if=boot.bin of={file_name1} bs=512 conv=notrunc count=1")
os.system(f"dd if=loader.bin of={file_name1} bs=512 conv=notrunc seek=1")
os.system(f"dd if=kernel.elf of={file_name1} bs=512 conv=notrunc seek=100")

# 处理 disk2.vhd
target_path = "k"

# 创建 diskpart 脚本文件来挂载磁盘
with open("a.txt", "w") as f:
    f.write(f"select vdisk file=\"{os.getcwd()}\\{file_name2}\"\n")
    f.write("attach vdisk\n")
    f.write("select partition 1\n")
    f.write(f"assign letter={target_path}\n")

# 执行 diskpart 挂载
os.system("diskpart /s a.txt")
os.remove("a.txt")

# 复制文件到挂载的磁盘
os.system(f"copy /Y *.elf {target_path}:\\" )

# 创建 diskpart 脚本文件来卸载磁盘
with open("a.txt", "w") as f:
    f.write(f"select vdisk file=\"{os.getcwd()}\\{file_name2}\"\n")
    f.write("detach vdisk\n")

# 执行 diskpart 卸载
os.system("diskpart /s a.txt")
os.remove("a.txt")