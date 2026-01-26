/*
 * ------------------------------------------------------------------------
 *   File Name: vbe.c
 *      Author: Zhao Yanbai
 *              2021-11-06 15:56:23 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <linkage.h>
// #include <page.h>
#include <system.h>
#include <types.h>

// VESA BIOS Extensions

// VGA
// VGA（Video Graphics Array）即视频图形阵列，是IBM在1987年随PS/2机推出的。
// VGA主要由七大块组成：图形控制器、显示存储器、定序器、CRT控制器、数据串行发生器、属性控制器和数模转换器DAC。

// VBE
// IBM的VGA标准是显示卡发展史上的一块丰碑。但后来无法满足人们的需要，于是市场上出现了TVGA、S3系列、Cirrus
// Logic、ET等为首的一批显示卡，提供了比VGA分辨率更高，颜色更丰富的显示模式，又兼容VGA显示卡，它们被统称为Super
// VGA（SVGA）。 各种不同的SVGA之间的显示控制各不相同，带来软件兼容性问题，为此视频电子学标准协会VESA（Video Electronics
// Standards Association）提出了一组附加的BIOS功能调用接口——VBE（VESA BIOS EXTENSION）标准，
// 从而在软件接口层次上实现了各种SVGA显示卡之间的兼容性。时至今日，所有的显示卡OEM厂商都提供了符合VESA
// SUPER标准的扩展BIOS 通过一组INT 10H中断调用（AH=4FH），可以方便地使用VESA
// SVGA的扩展功能而不必了解各种显示卡的硬件细节。 Super VGA 的扩充显示能力关键取决于对较大显示存储器的寻址能力。 各Super
// VGA 卡提供的分辨率远高于VGA，VESA VBE均赋予一个标准的16位模式号（实际上是9位，其他各位为标志位或留给以后扩充）。

// typedef uint8_t BCD;
// typedef struct vg_vbe_contr_info {
//     char VBESignature[4];
//     BCD VBEVersion[2];
//     char *OEMString;
//     uint16_t *VideoModeList;
//     uint32_t TotalMemory;
//     char *OEMVendorNamePtr;
//     char *OEMProductNamePtr;
//     char *OEMProductRevPtr;
// } vg_vbe_contr_info_t;

#define VBE_MODE_END 0xFFFF
typedef struct segoff {
    uint16_t offset;
    uint16_t segment;
} __attribute__((packed)) segoff_t;

// typedef uint32_t segoff_t;
typedef struct vbe_controller_info {
    uint8_t signature[4];  // 'VESA'
    uint16_t version;      // BCD
    // uint8_t major_version;  // BCD
    // uint8_t minor_version;  // BCD
    segoff_t oem_string_ptr;
    uint8_t capabilities[4];
    segoff_t video_mode_ptr;
    uint16_t total_memory;  // Number of 64kB memory blocks.
    // ... 更多 VBE 3.0+ 字段
    uint16_t oem_software_rev;
    segoff_t oem_vendor_name_ptr;
    segoff_t oem_product_name_ptr;
    segoff_t oem_product_rev_ptr;
    uint8_t reserved[222];
} __attribute__((packed)) vbe_controller_info_t;

typedef struct vbe_mode_info {
    u16 mode_attributes;
    u8 wina_attributes;
    u8 winb_attributes;
    u16 win_granularity;
    u16 win_size;
    u16 wina_segment;
    u16 winb_segment;
    u32 win_func_ptr;
    u16 bytes_per_scan_line;
    u16 x_resolution;
    u16 y_resolution;
    u8 xchar_size;
    u8 ychar_size;
    u8 number_of_planes;
    u8 bits_per_pixel;
    u8 number_of_banks;
    u8 memory_model;
    u8 bank_size;
    u8 number_of_image_pages;
    u8 _reserved0;
    u8 red_mask_size;
    u8 red_field_position;
    u8 green_mask_size;
    u8 green_field_position;
    u8 blue_mask_size;
    u8 blue_field_position;
    u8 rsvd_mask_size;
    u8 rsvd_field_position;
    u8 direct_color_mode_info;
    u32 phys_base_ptr;
    u32 _reserved1;
    u16 _reserved2;
    u16 lin_bytes_per_scan_line;
    u8 bank_number_of_image_pages;
    u8 lin_number_of_image_pages;
    u8 lin_red_mask_size;
    u8 lin_red_field_position;
    u8 lin_green_mask_size;
    u8 lin_green_field_position;
    u8 lin_blue_mask_size;
    u8 lin_blue_field_position;
    u8 lin_rvsd_mask_size;
    u8 lin_rvsd_field_position;
    u32 max_pixel_clock;
    u8 _reserved3[189];
} __attribute__((packed)) vbe_mode_info_t;

// pte_t vbe_pte[PTECNT_PER_PAGE] __attribute__((__aligned__(PAGE_SIZE)));

uint32_t segoff_to_addr(segoff_t sg) {
    return 0xC0000000 + (sg.segment << 4) + sg.offset;
}

vbe_mode_info_t* get_vbe_mode_info(uint16_t mode) {
    // 设置并执行实模式代码以调用 VBE 函数 0x4F01
    // 返回指向 VBE 模式信息的指针
    return NULL;
}
// extern pde_t init_pgd[];
void init_vbe(void* vciptr, void* vmiptr) {
    // VBE是传统BIOS接口，在现代系统上可能没有被正确初始化
    // 也无法在保护模式下调用VBE BIOS功能
    // 因此这里只是打印VBE信息
    // 更可靠的数据是 MULTIBOOT_TAG_TYPE_FRAMEBUFFER 的数据
    vbe_controller_info_t* vci = vciptr;
    vbe_mode_info_t* vmi = (vbe_mode_info_t*)vmiptr;

    printk("VBE[deprecated]:\n");
    printk("Signature %c%c%c%c\n", vci->signature[0], vci->signature[1], vci->signature[2], vci->signature[3]);
    printk("version %04x\n", vci->version);
    printk("total memory %u x 64K\n", vci->total_memory);
    printk("OEM software rev: %04X\n", vci->oem_software_rev);
    printk("OEM vendor: %s\n", segoff_to_addr(vci->oem_vendor_name_ptr));
    printk("OEM product name: %s\n", segoff_to_addr(vci->oem_product_name_ptr));
    printk("OEM product rev: %s\n", segoff_to_addr(vci->oem_product_rev_ptr));

    printk("SEG %04X OFFSET %04X\n", vci->video_mode_ptr.segment, vci->video_mode_ptr.offset);
    uint16_t* modes = (uint16_t*)segoff_to_addr(vci->video_mode_ptr);
    printk("vbe modes:");
    while (*modes != VBE_MODE_END) {
        printk(" %04x", *modes);
        vbe_mode_info_t* mi = get_vbe_mode_info(*modes);

        // 目前以下判断不会生效
        if ((mi != 0) && (mi->mode_attributes & 0x01)) {
            printk("R %u x %u\n", mi->x_resolution, mi->y_resolution);
        }

        modes++;
    }
    printk("\n");

    printk("vbe[deprecated] phys addr %08x resolution %u x %u\n", vmi->phys_base_ptr, vmi->x_resolution,
           vmi->y_resolution);
#if 0
    system.vbe_phys_addr = vmi->phys_base_ptr;
    system.x_resolution = vmi->x_resolution;
    system.y_resolution = vmi->y_resolution;
#endif
}
