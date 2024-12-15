# OS final experiment note

贾松儒-2022302181108

Here I recorded my own part in this experiment project

## work checklist

![image-20241204125053027](./assets/image-20241204125053027.png)

![image-20241204125018976](./assets/image-20241204125018976.png)

![image-20241204125131337](./assets/image-20241204125131337.png)

![image-20241204125137371](./assets/image-20241204125137371.png)

## 使用fat12文件系统的软盘启动mini-os

文件见`boot/*`,因为采用的原os镜像已经很详尽地完成了从软盘启动mini-os的逻辑代码,这里进行技术总结.

`boot/boot.asm`

由于引导扇区大小固定512Byte,将重要的逻辑代码放在loader.asm中,boot.asm只起到发现并加载loader.bin的数据到内存中并跳转的作用

启动时,BIOS将引导扇区加载到内存中0:7c00h处执行剩余的引导代码

line14:`	org  07c00h			; Boot 状态, Bios 将把 Boot Sector 加载到 0:7C00 处并开始执行`

line33

```asm
LABEL_START:	
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, BaseOfStack

	; 清屏
	mov	ax, 0600h		; AH = 6,  AL = 0h
	mov	bx, 0700h		; 黑底白字(BL = 07h)
	mov	cx, 0			; 左上角: (0, 0)
	mov	dx, 0184fh		; 右下角: (80, 50)
	int	10h			; int 10h

	mov	dh, 0			; "Booting  "
	call	DispStr			; 显示字符串
	
	xor	ah, ah	; ┓
	xor	dl, dl	; ┣ 软驱复位
	int	13h	; ┛
```

调整堆栈基地址,调整屏幕显示(在启动过程中会在屏幕中打印启动相关信息),因为使用软盘启动首先复位A盘软驱的磁盘系统,确保后续磁盘读写功能正常进行.

接着在FAT12文件系统的A盘中通过文件名匹配来找到目录区中LOADER.BIN文件的索引(8+3).找到后读取LOADER.BIN的所有内容到内存中并跳转到LOADER.BIN开始执行.(line54)

`boot/loader.asm`

程序以实模式启动,跳转到line44 `LABEL_START`中进行初始化,设置数据段寄存器（`DS`、`ES` 等）指向当前代码段，初始化堆栈指针.(line44)

进行内存检测,使用`int 15h`中断地`eax = E820h`功能检测系统内存分布,将结果保存在MemChkBuf中.(line54)

在FAT12文件系统的软驱A搜索KERNEL.BIN内核文件并加载到内存的固定位置(如果未找到，显示“No Kernel”消息并进入死循环,如果内核文件过大，显示“Too Large”消息并进入死循环)(line72)

将硬盘主引导扇区(MBR)内容读入0500h(line208)

接着为了切换到保护模式,进行加载全局描述符表GDT,开A20,切CR0等逻辑进入保护模式.

进入保护模式后,初始化内核,遍历内核的 ELF 程序头表，将内核各段加载到合适的位置,启动分页实现虚拟内存管理,开CR3

另外,在加载的过程中,很贴心地给出了各种调试信息打印在屏幕上.



