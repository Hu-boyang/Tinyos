import os

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

os.system(f"dd if=boot.bin of={file_name1} bs=512 conv=notrunc count=1")
os.system(f"dd if=loader.bin of={file_name1} bs=512 conv=notrunc seek=1")
os.system(f"dd if=kernel.elf of={file_name1} bs=512 conv=notrunc seek=100")
os.system(f"dd if=shell.elf of={file_name1} bs=512 conv=notrunc seek=5000")
