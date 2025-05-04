#include "loader.h"

// 以LBA读取磁盘函数，读取的是kernel.elf
// sector 是读取的扇区 sector_count 是读取的扇区数量 buf是读取的位置
static void read_disk(int sector, int sector_count, uint8_t * buf) {
    outb(0x1F6, (uint8_t) (0xE0));

	outb(0x1F2, (uint8_t) (sector_count >> 8));
    outb(0x1F3, (uint8_t) (sector >> 24));		
    outb(0x1F4, (uint8_t) (0));					
    outb(0x1F5, (uint8_t) (0));					

    outb(0x1F2, (uint8_t) (sector_count));
	outb(0x1F3, (uint8_t) (sector));			
	outb(0x1F4, (uint8_t) (sector >> 8));		
	outb(0x1F5, (uint8_t) (sector >> 16));		

	outb(0x1F7, (uint8_t) 0x24);

	
	uint16_t *data_buf = (uint16_t*) buf;
	// 这里是要读取的扇区
	while (sector_count-- > 0) {
		
		while ((inb(0x1F7) & 0x88) != 0x8) {}

		// 以每次2字节方式读取每个扇区
		for (int i = 0; i < SECTOR_SIZE / 2; i++) {
			*data_buf++ = inw(0x1F0);
		}
	}
}

// 从1M处读取elf文件并加载到0x1000处
static uint32_t reload_elf_file(uint8_t* file_buffer){
	Elf32_Ehdr* elf_hdr=(Elf32_Ehdr*)file_buffer;
	// 检查elf头是否正确
	if((elf_hdr->e_ident[0]!=0x7f)||(elf_hdr->e_ident[1]!='E')
		|| (elf_hdr->e_ident[2]!='L')||(elf_hdr->e_ident[3]!='F')){
			return 0;
	}
	// 加载各个段
	for(int i=0;i<elf_hdr->e_phnum;i++){
		// 取出各个表项，注意这里先对其强制转换在加的i这样偏移就是一个Elf_Phdr
		// i就相当于数组索引
		Elf32_Phdr* phdr=(Elf32_Phdr*)(file_buffer+elf_hdr->e_phoff)+i;

		// PT_LOAD代表是可加载类型
		if(phdr->p_type!=PT_LOAD){
			continue;
		}

		// 这里的p_paddr即要放入物理地址在kernel.lds指明
		// 相当于在编译生成.elf文件时p_paddr就已经设置到了0x1000那个范围的位置了
		uint8_t* src=file_buffer+phdr->p_offset;
		uint8_t* dest=(uint8_t*) phdr->p_paddr;
		for(int j=0;j<phdr->p_filesz;j++){
			*dest++=*src++;
		}
		dest=(uint8_t*)phdr->p_paddr+phdr->p_filesz;
		
		// 将多出的空间设置为0
		for(int j=0;j<phdr->p_memsz-phdr->p_filesz;j++){
			*dest++=0;
		}
	}
	return elf_hdr->e_entry;
}

//终止函数
static void die(int code){
	for(;;){

	}
}

void load_kernel(void){
	// (uint8_t*)指针是32位的四字节不是一字节别弄晕了
	// 读取kernel.elf把他放在1MB的位置 kernel大小为250kb
    read_disk(100,500,(uint8_t*)SYS_KERNEL_LOAD_ADDR);
	uint32_t kernel_entry=reload_elf_file((uint8_t*)SYS_KERNEL_LOAD_ADDR);
	if(kernel_entry==0){
		die(-1);
	}
	// 这里boot_info传递到kernel/Start.S
    ((void (*)(boot_info_t*))kernel_entry)(&boot_info);
}