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

// extern pde_t init_pgd[];
void init_vbe(void *vciptr, void *vmiptr) {
    vbe_mode_info_t *vmi = (vbe_mode_info_t *)vmiptr;

    system.vbe_phys_addr = vmi->phys_base_ptr;
    system.x_resolution = vmi->x_resolution;
    system.y_resolution = vmi->y_resolution;
}