/*
 * ------------------------------------------------------------------------
 *   File Name: multiboot2_header.c
 *      Author: Zhao Yanbai
 *              2025-12-30 08:48:35 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */



#include <multiboot2.h>


// 启用FB这个功能，需要修改grub.cfg，添加如下内容
// load_video
// set gfxmode=auto
// set gfxpayload=keep
// terminal_output gfxterm
#define ENABLE_FB 0

#define ALIGN8 __attribute__((aligned(8)))

typedef struct ALIGN8 multiboot2_elf_header {
    ALIGN8 struct multiboot_header header;
#if ENABLE_FB
    ALIGN8 struct multiboot_header_tag_framebuffer fb;
#endif
    ALIGN8 struct multiboot_header_tag end;
} multiboot2_bin_header_t;



__attribute__((section(".multiboot2_header"), used))
const multiboot2_bin_header_t multiboot2_elf_header = {
    .header = {
        .magic = MULTIBOOT2_HEADER_MAGIC,
        .architecture = MULTIBOOT_ARCHITECTURE_I386,
        .header_length = sizeof(multiboot2_bin_header_t),
        .checksum = -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + sizeof(multiboot2_bin_header_t)),
    },
#if ENABLE_FB
    .fb = {
        .type = MULTIBOOT_HEADER_TAG_FRAMEBUFFER,
        .flags = MULTIBOOT_HEADER_TAG_OPTIONAL,
        .size = sizeof(struct multiboot_header_tag_framebuffer),
        .width = 1024,
        .height = 768,
        .depth = 32,
    },
#endif
    .end = {
        // tags are terminated by a tag of type ‘0’ and size ‘8’.
        // MULTIBOOT_HEADER_TAG_END == 0
        // sizeof(struct multiboot_header_tag) == 8
        .type = MULTIBOOT_HEADER_TAG_END,
        .flags = 0,
        .size = sizeof(struct multiboot_header_tag),
    },
};
