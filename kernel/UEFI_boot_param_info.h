#ifndef __UEFI_BOOT_PARAM_INFO_H__

#define __UEFI_BOOT_PARAM_INFO_H__

struct EFI_GRAPHICS_OUTPUT_INFORMATION
{
	unsigned int HorizontalResolution;
	unsigned int VerticalResolution;
	unsigned int PixelsPerScanLine;

	unsigned long FrameBufferBase;
	unsigned long FrameBufferSize;
};

struct EFI_E820_MEMORY_DESCRIPTOR
{
	unsigned long address;
	unsigned long length;
	unsigned int  type;
}__attribute__((packed));

struct EFI_E820_MEMORY_DESCRIPTOR_INFORMATION
{
	unsigned int E820_Entry_count;
	struct EFI_E820_MEMORY_DESCRIPTOR E820_Entry[0];
};

struct KERNEL_BOOT_PARAMETER_INFORMATION
{
	struct EFI_GRAPHICS_OUTPUT_INFORMATION Graphics_Info;
	struct EFI_E820_MEMORY_DESCRIPTOR_INFORMATION E820_Info;
};

extern struct KERNEL_BOOT_PARAMETER_INFORMATION *boot_para_info;

#endif
