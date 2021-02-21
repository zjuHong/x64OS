#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>
/*屏幕显示信息结构*/
struct EFI_GRAPHICS_OUTPUT_INFORMATION
{
	unsigned int HorizontalResolution;
	unsigned int VerticalResolution;
	unsigned int PixelsPerScanLine;

	unsigned long FrameBufferBase;
	unsigned long FrameBufferSize;
};
/*内存信息单元结构*/
struct EFI_E820_MEMORY_DESCRIPTOR
{
	unsigned long address;
	unsigned long length;
	unsigned int  type;
}__attribute__((packed));
/*内存信息结构*/
struct EFI_E820_MEMORY_DESCRIPTOR_INFORMATION
{
	unsigned int E820_Entry_count;
	struct EFI_E820_MEMORY_DESCRIPTOR E820_Entry[0];
};
/*启动信息整合结构*/
struct KERNEL_BOOT_PARAMETER_INFORMATION
{
	struct EFI_GRAPHICS_OUTPUT_INFORMATION Graphics_Info;
	struct EFI_E820_MEMORY_DESCRIPTOR_INFORMATION E820_Info;
};
/** 
 * @brief uefi主函数
 * @param 
 * @return 
 */
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_LOADED_IMAGE        *LoadedImage;
	EFI_FILE_IO_INTERFACE   *Vol;
	EFI_FILE_HANDLE         RootFs;
	EFI_FILE_HANDLE         FileHandle;

	int i = 0;
	void (*func)(void);
	EFI_STATUS status = EFI_SUCCESS;
	struct KERNEL_BOOT_PARAMETER_INFORMATION *kernel_boot_para_info = NULL;

//////////////////////ESP分区中读取文件里的数据
	gBS->HandleProtocol(ImageHandle,&gEfiLoadedImageProtocolGuid,(VOID*)&LoadedImage);
	//使用LocateProtocol方法取得该协议（GUID值为gEfiDevicePathToTextProtocolGuid）对应的接口
	gBS->HandleProtocol(LoadedImage->DeviceHandle,&gEfiSimpleFileSystemProtocolGuid,(VOID*)&Vol);
	Vol->OpenVolume(Vol,&RootFs);//借助OpenVolume方法打开ESP分区，并返回分区句柄
	status = RootFs->Open(RootFs,&FileHandle,(CHAR16*)L"kernel.bin",EFI_FILE_MODE_READ,0);//读取内核函数
	if(EFI_ERROR(status))
	{
		Print(L"Open kernel.bin Failed.\n");
		return status;
	}

	EFI_FILE_INFO* FileInfo;
	UINTN BufferSize = 0;
	EFI_PHYSICAL_ADDRESS pages = 0x100000;//如果此文件存在，则将文件里的数据读取到物理地址0x100000处，并将文件的基础信息打印到屏幕上

	BufferSize = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 100;
	gBS->AllocatePool(EfiRuntimeServicesData,BufferSize,(VOID**)&FileInfo); 
	//取得文件的长度、创建时间、属性等基础信息
	FileHandle->GetInfo(FileHandle,&gEfiFileInfoGuid,&BufferSize,FileInfo);
	Print(L"\tFileName:%s\t Size:%d\t FileSize:%d\t Physical Size:%d\n",FileInfo->FileName,FileInfo->Size,FileInfo->FileSize,FileInfo->PhysicalSize);
	//从物理地址0x100000处按文件占用的物理内存页面（每个物理内存页面大小为4KB）数量分配页面
	gBS->AllocatePages(AllocateAddress,EfiLoaderData,(FileInfo->FileSize + 0x1000 - 1) / 0x1000,&pages);
	Print(L"Read Kernel File to Memory Address:%018lx\n",pages);
	BufferSize = FileInfo->FileSize;
	//Read方法读取kernel.bin文件保存的数据
	FileHandle->Read(FileHandle,&BufferSize,(VOID*)pages);
	//清理工作
	gBS->FreePool(FileInfo);
	FileHandle->Close(FileHandle);
	RootFs->Close(RootFs);

///////////////////显示设置
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gGraphicsOutput = 0;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info = 0;
	UINTN InfoSize = 0;

	pages = 0x60000;
	kernel_boot_para_info = (struct KERNEL_BOOT_PARAMETER_INFORMATION *)0x60000;//将内存信息存到地址0x60000处
	gBS->AllocatePages(AllocateAddress,EfiLoaderData,1,&pages);
	gBS->SetMem((void*)kernel_boot_para_info,0x1000,0);
	//使用LocateProtocol接口获取显示器信息
	gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,NULL,(VOID **)&gGraphicsOutput);

	long H_V_Resolution = gGraphicsOutput->Mode->Info->HorizontalResolution * gGraphicsOutput->Mode->Info->VerticalResolution;
	int MaxResolutionMode = gGraphicsOutput->Mode->Mode;

	for(i = 0;i < gGraphicsOutput->Mode->MaxMode;i++)
	{//遍历图形设备支持的所有显示模式，找到分辨率最大的模式
		gGraphicsOutput->QueryMode(gGraphicsOutput,i,&InfoSize,&Info);
		if((Info->PixelFormat == 1) && (Info->HorizontalResolution * Info->VerticalResolution > H_V_Resolution))
		{
			H_V_Resolution = Info->HorizontalResolution * Info->VerticalResolution;
			MaxResolutionMode = i;
		}
		gBS->FreePool(Info);//释放内存
	}

	gGraphicsOutput->SetMode(gGraphicsOutput,MaxResolutionMode);//设置显示模式
	gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,NULL,(VOID **)&gGraphicsOutput);
	Print(L"Current Mode:%02d,Version:%x,Format:%d,Horizontal:%d,Vertical:%d,ScanLine:%d,FrameBufferBase:%018lx,FrameBufferSize:%018lx\n",gGraphicsOutput->Mode->Mode,gGraphicsOutput->Mode->Info->Version,gGraphicsOutput->Mode->Info->PixelFormat,gGraphicsOutput->Mode->Info->HorizontalResolution,gGraphicsOutput->Mode->Info->VerticalResolution,gGraphicsOutput->Mode->Info->PixelsPerScanLine,gGraphicsOutput->Mode->FrameBufferBase,gGraphicsOutput->Mode->FrameBufferSize);
	//将显示信息复制到全局结构
	kernel_boot_para_info->Graphics_Info.HorizontalResolution = gGraphicsOutput->Mode->Info->HorizontalResolution;
	kernel_boot_para_info->Graphics_Info.VerticalResolution = gGraphicsOutput->Mode->Info->VerticalResolution;
	kernel_boot_para_info->Graphics_Info.PixelsPerScanLine = gGraphicsOutput->Mode->Info->PixelsPerScanLine;
	kernel_boot_para_info->Graphics_Info.FrameBufferBase = gGraphicsOutput->Mode->FrameBufferBase;
	kernel_boot_para_info->Graphics_Info.FrameBufferSize = gGraphicsOutput->Mode->FrameBufferSize;

	Print(L"Map Graphics FrameBufferBase to Virtual Address 0xffff800003000000\n");
	long * PageTableEntry = (long *)0x103000;
	for(i = 0;i < (gGraphicsOutput->Mode->FrameBufferSize + 0x200000 - 1) >> 21;i++)	// map to virtual address 0xffff800003000000
	{	//FrameBuffer的长度更新页表中的FrameBuffer地址映射。这里将FrameBuffer地址映射到线性地址0xffff800003000000处
		*(PageTableEntry + 24 + i) = gGraphicsOutput->Mode->FrameBufferBase | 0x200000 * i | 0x87;
		Print(L"Page %02d,Address:%018lx,Value:%018lx\n",i,(long)(PageTableEntry + 24 + i),*(PageTableEntry + 24 + i));
	}

///////////////////////////内存信息获取
	struct EFI_E820_MEMORY_DESCRIPTOR *E820p = kernel_boot_para_info->E820_Info.E820_Entry;
	struct EFI_E820_MEMORY_DESCRIPTOR *LastE820 = NULL;
	unsigned long LastEndAddr = 0;
	int E820Count = 0;

	UINTN MemMapSize = 0;
	EFI_MEMORY_DESCRIPTOR* MemMap = 0;
	UINTN MapKey = 0;
	UINTN DescriptorSize = 0;
	UINT32 DesVersion = 0;
	// GetMemoryMap方法来查询物理地址空间信息占用的内存空间大小，取得内存描述符结构数组存储空间的长度
	gBS->GetMemoryMap(&MemMapSize,MemMap,&MapKey,&DescriptorSize,&DesVersion);
	MemMapSize += DescriptorSize * 5;
	//调用引导服务的AllocatePool方法为内存描述符结构数组分配存储空间
	gBS->AllocatePool(EfiRuntimeServicesData,MemMapSize,(VOID**)&MemMap);
	Print(L"Get MemMapSize:%d,DescriptorSize:%d,count:%d\n",MemMapSize,DescriptorSize,MemMapSize/DescriptorSize);
	gBS->SetMem((void*)MemMap,MemMapSize,0);
	//再次调用GetMemoryMap方法获取物理地址空间信息
	status = gBS->GetMemoryMap(&MemMapSize,MemMap,&MapKey,&DescriptorSize,&DesVersion);
	Print(L"Get MemMapSize:%d,DescriptorSize:%d,count:%d\n",MemMapSize,DescriptorSize,MemMapSize/DescriptorSize);
	if(EFI_ERROR(status))
		Print(L"Get GetMemoryMap status:%018lx\n",status);

	Print(L"Get EFI_MEMORY_DESCRIPTOR Structure:%018lx\n",MemMap);

	for(i = 0;i < MemMapSize / DescriptorSize;i++)
	{
		int MemType = 0;
		EFI_MEMORY_DESCRIPTOR* MMap = (EFI_MEMORY_DESCRIPTOR*) ((CHAR8*)MemMap + i * DescriptorSize);
		if(MMap->NumberOfPages == 0)
			continue;
//		Print(L"MemoryMap %4d %10d (%16lx<->%16lx) %016lx\n",MMap->Type,MMap->NumberOfPages,MMap->PhysicalStart,MMap->PhysicalStart + (MMap->NumberOfPages << 12),MMap->Attribute);
		switch(MMap->Type)
		{
			case EfiReservedMemoryType:
			case EfiMemoryMappedIO:
			case EfiMemoryMappedIOPortSpace:
			case EfiPalCode:
				MemType= 2;	//2:ROM or Reserved
				break;

			case EfiUnusableMemory:
				MemType= 5;	//5:Unusable
				break;

			case EfiACPIReclaimMemory:
				MemType= 3;	//3:ACPI Reclaim Memory
				break;

			case EfiLoaderCode:
			case EfiLoaderData:
			case EfiBootServicesCode:
			case EfiBootServicesData:
			case EfiRuntimeServicesCode:
			case EfiRuntimeServicesData:
			case EfiConventionalMemory:
			case EfiPersistentMemory:
				MemType= 1;	//1:RAM
				break;

			case EfiACPIMemoryNVS:
				MemType= 4;	//4:ACPI NVS Memory
				break;

			default:
				Print(L"Invalid UEFI Memory Type:%4d\n",MMap->Type);
				continue;
		}
		//将物理地址空间信息根据地址空间类型转换成E820内存结构，
		if((LastE820 != NULL) && (LastE820->type == MemType) && (MMap->PhysicalStart == LastEndAddr))
		{
			LastE820->length += MMap->NumberOfPages << 12;
			LastEndAddr += MMap->NumberOfPages << 12;
		}
		else
		{
			E820p->address = MMap->PhysicalStart;
			E820p->length = MMap->NumberOfPages << 12;
			E820p->type = MemType;
			LastEndAddr = MMap->PhysicalStart + (MMap->NumberOfPages << 12);
			LastE820 = E820p;
			E820p++;
			E820Count++;			
		}
	}

	kernel_boot_para_info->E820_Info.E820_Entry_count = E820Count;
	LastE820 = kernel_boot_para_info->E820_Info.E820_Entry;
	int j = 0;
	for(i = 0; i< E820Count; i++)
	{
		struct EFI_E820_MEMORY_DESCRIPTOR* e820i = LastE820 + i;
		struct EFI_E820_MEMORY_DESCRIPTOR MemMap;
		for(j = i + 1; j< E820Count; j++)
		{
			struct EFI_E820_MEMORY_DESCRIPTOR* e820j = LastE820 + j;
			if(e820i->address > e820j->address)
			{
				MemMap = *e820i;
				*e820i = *e820j;
				*e820j = MemMap;
			}
		}
	}
	//并将转换后的信息保存到KERNEL_BOOT_PARAMETER_INFORMATION结构里
	LastE820 = kernel_boot_para_info->E820_Info.E820_Entry;
	for(i = 0;i < E820Count;i++)
	{
		Print(L"MemoryMap (%10lx<->%10lx) %4d\n",LastE820->address,LastE820->address+LastE820->length,LastE820->type);
		LastE820++;
	}
	gBS->FreePool(MemMap);

	Print(L"Call ExitBootServices And Jmp to Kernel.\n");

	gBS->GetMemoryMap(&MemMapSize,MemMap,&MapKey,&DescriptorSize,&DesVersion);
	//关闭所有之前打开的协议接口句柄
	gBS->CloseProtocol(LoadedImage->DeviceHandle,&gEfiSimpleFileSystemProtocolGuid,ImageHandle,NULL);
	gBS->CloseProtocol(ImageHandle,&gEfiLoadedImageProtocolGuid,ImageHandle,NULL);

	gBS->CloseProtocol(gGraphicsOutput,&gEfiGraphicsOutputProtocolGuid,ImageHandle,NULL);

/////////////////////把处理器的控制权移交给系统内核
	status = gBS->ExitBootServices(ImageHandle,MapKey);//ExitBootServices方法退出引导服务
	if(EFI_ERROR(status))
	{
		Print(L"ExitBootServices: Failed, Memory Map has Changed.\n");
		return EFI_INVALID_PARAMETER;
	}
	func = (void *)0x100000;//跳转至物理地址0x100000处执行系统内核程序
	func();

	return EFI_SUCCESS;
}
