/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: yy
 *
 * Created on 2016年2月27日, 上午11:49
 */

#include "cmdline.h"
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "lzma_compress.h"

using namespace std;

#define BigtoLittle16(x)    ((((uint16_t)(x) & 0xff00) >> 8) | (((uint16_t)(x) & 0x00ff) << 8))
#define BigtoLittle32(x)    ((((uint32_t)(x) & 0xff000000) >> 24) | \
                            (((uint32_t)(x) & 0x00ff0000) >> 8) | \
                            (((uint32_t)(x) & 0x0000ff00) << 8) | \
                            (((uint32_t)(x) & 0x000000ff) << 24))

#define IH_MAGIC   0x56190527  /* Image Magic Number       */
#define IH_NMLEN   32   /* Image Name Length        */

typedef struct image_header
{
    uint32_t ih_magic; /* Image Header Magic Number     */
    uint32_t ih_hcrc; /* Image Header CRC Checksum     */
    uint32_t ih_time; /* Image Creation Timestamp */
    uint32_t ih_size; /* Image Data Size     */
    uint32_t ih_load; /* Data Load Address         */
    uint32_t ih_ep; /* Entry Point Address      */
    uint32_t ih_dcrc; /* Image Data CRC Checksum */
    uint8_t ih_os; /* Operating System         */
    uint8_t ih_arch; /* CPU architecture         */
    uint8_t ih_type; /* Image Type          */
    uint8_t ih_comp; /* Compression Type         */
    uint8_t ih_name[IH_NMLEN]; /* Image Name      */
} image_header_t;

static image_header_t header;
extern unsigned int crc32(const unsigned char *buf, unsigned int size);

uint8_t get_value_arch(string str)
{
    uint8_t arch = 0;

    if (0 == str.compare("alpha"))arch = 1;
    else if (0 == str.compare("arm"))arch = 2;
    else if (0 == str.compare("x86"))arch = 3;
    else if (0 == str.compare("ia64"))arch = 4;
    else if (0 == str.compare("mips"))arch = 5;
    else if (0 == str.compare("mips64"))arch = 6;
    else if (0 == str.compare("ppc"))arch = 7;
    else if (0 == str.compare("s390"))arch = 8;
    else if (0 == str.compare("sh"))arch = 9;
    else if (0 == str.compare("sparc"))arch = 10;
    else if (0 == str.compare("sparc64"))arch = 11;
    else if (0 == str.compare("m68k"))arch = 12;

    return (arch);
}

uint8_t get_value_os(string str)
{
    uint8_t os = 0;

    if (0 == str.compare("openbsd"))os = 1;
    else if (0 == str.compare("netbsd"))os = 2;
    else if (0 == str.compare("freebsd"))os = 3;
    else if (0 == str.compare("4_4bsd"))os = 4;
    else if (0 == str.compare("linux"))os = 5;
    else if (0 == str.compare("svr4"))os = 6;
    else if (0 == str.compare("esix"))os = 7;
    else if (0 == str.compare("solaris"))os = 8;
    else if (0 == str.compare("irix"))os = 9;
    else if (0 == str.compare("sco"))os = 10;
    else if (0 == str.compare("dell"))os = 11;
    else if (0 == str.compare("ncr"))os = 12;
    else if (0 == str.compare("lynxos"))os = 13;
    else if (0 == str.compare("vxworks"))os = 14;
    else if (0 == str.compare("psos"))os = 15;
    else if (0 == str.compare("qnx"))os = 16;
    else if (0 == str.compare("u-boot"))os = 17;
    else if (0 == str.compare("rtems"))os = 18;
    else if (0 == str.compare("artos"))os = 19;
    else if (0 == str.compare("rtthread"))os = 20;

    return (os);
}

uint8_t get_value_type(string str)
{
    uint8_t type;

    if (0 == str.compare("standalone"))type = 1;
    else if (0 == str.compare("kernel"))type = 2;
    else if (0 == str.compare("ramdisk"))type = 3;
    else if (0 == str.compare("multi"))type = 4;
    else if (0 == str.compare("firmware"))type = 5;
    else if (0 == str.compare("script"))type = 6;    
    else if (0 == str.compare("filesystem"))type = 7;
    else if (0 == str.compare("diffpatch"))type = 8;
    
    return (type);
}

uint8_t get_value_comp(string str)
{
    uint8_t comp = 0;

    if (0 == str.compare("none"))comp = 0;
    else if (0 == str.compare("gzip"))comp = 1;
    else if (0 == str.compare("bzip2"))comp = 2;
    else if (0 == str.compare("lzma"))comp = 3;

    return (comp);
}

unsigned int file_get_length(FILE *fp)
{
    long cur_pos;
    unsigned int len = 0;

    //取得当前文件流的读取位置
    cur_pos = ftell(fp);
    //将文件流的读取位置设为文件末尾
    fseek(fp, 0, SEEK_END);
    //获取文件末尾的读取位置,即文件大小
    len = ftell(fp);
    //将文件流的读取位置还原为原先的值
    fseek(fp, cur_pos, SEEK_SET);

    return len;
}

unsigned char* file_read(const char *path, unsigned int *len)
{
    FILE *fp;
    unsigned char *dp = NULL;
    unsigned int file_len;

    fp = fopen(path, "rb");
    if (fp != NULL)
    {
        file_len = file_get_length(fp);
        *len = file_len;
        dp = (unsigned char*) malloc(file_len + 1);
        if (dp != NULL)
        {
            fread(dp, 1, file_len, fp);
        }
        else printf("malloc error!\n");
    }
    fclose(fp);

    return (dp);
}

void file_write(const char *path, void *buffer, unsigned int len)
{
    FILE *fp;
    fp = fopen(path, "wb");
    if (fp != NULL)
    {
        fwrite(buffer, len, 1, fp);
    }
    fclose(fp);
}

int mkuimage(const char *path, const char *out_path)
{
    unsigned int image_len, zimage_len, uzimage_len;
    unsigned int hcrc, dcrc, tm;
    unsigned char *image, *zimage, *uzimage;
    time_t t;

    time(&t);
    tm = (unsigned int) t;
    image = file_read(path, &image_len);
    if (image != NULL)
    {
        //压缩
        switch (header.ih_comp)
        {
        case 0: //不压缩
        case 1: //gzip,不支持
        case 2: //bzip2,不支持
        default:
            header.ih_comp = 0;
            zimage_len = image_len;
            uzimage_len = zimage_len + sizeof (image_header_t);
            zimage = image;
            uzimage = (unsigned char*) malloc(uzimage_len + 1);
            memcpy(uzimage + sizeof (image_header_t), zimage, zimage_len);
            free(image);
            break;

        case 3: //lzma
            zimage_len = image_len + 10 * 1024;
            zimage = (unsigned char*) malloc(zimage_len);
            lzma_compress(zimage, (int*)&zimage_len, image, (int)image_len, 12, 9);
            uzimage_len = zimage_len + sizeof (image_header_t);
            uzimage = (unsigned char*) malloc(uzimage_len + 1);
            memcpy(uzimage + sizeof (image_header_t), zimage, zimage_len);
            free(image);
            free(zimage);
            break;
        }

        dcrc = crc32(uzimage + sizeof (image_header_t), zimage_len);
        header.ih_magic = IH_MAGIC;
        header.ih_hcrc = 0;
        header.ih_time = BigtoLittle32(tm);
        header.ih_size = BigtoLittle32(zimage_len);
        header.ih_dcrc = BigtoLittle32(dcrc);

        hcrc = crc32((unsigned char*) &header, sizeof (image_header_t));
        header.ih_hcrc = BigtoLittle32(hcrc);
        memcpy(uzimage, &header, sizeof (image_header_t));

        file_write(out_path, uzimage, uzimage_len);
        cout << "creat output file." << endl;
        free(uzimage);
        return (0);
    }
    return (-1);
}

int main(int argc, char *argv[])
{
    // 创建一个命令行解析器
    cmdline::parser a;
    // 添加指定类型的输入参数
    // 第一个参数：长名称
    // 第二个参数：短名称（'\0'表示没有短名称）
    // 第三个参数：参数描述
    // 第四个参数：bool值，表示该参数是否必须存在（可选，默认值是false）
    // 第五个参数：参数的默认值（可选，当第四个参数为false时该参数有效）
    a.add<string>("arch", 'A', "set architecture to 'arch'", false, "arm");
    a.add<string>("os", 'O', "set operating system to 'os'", false, "linux");
    a.add<string>("type", 'T', "set image type to 'type'", false, "kernel", cmdline::oneof<string>("standalone", "kernel", "ramdisk", "multi", "firmware", "script", "filesystem", "diffpatch"));
    a.add<string>("comp", 'C', "set compression type 'comp'", false, "none", cmdline::oneof<string>("none", "gzip", "bzip2", "lzma"));
    a.add<string>("addr", 'a', "set load address to 'addr' (hex)", false, "0x0800FFC0");
    a.add<string>("ep", 'e', "set entry point to 'ep' (hex)", false, "0x08010000");
    a.add<string>("name", 'n', "set image name to 'name'", false, "kernel");
    a.add<string>("input", 'i', "input file name", true, "");
    a.add<string>("output", 'o', "output file name", true, "");
    a.add("x", '\0', "set XIP (execute in place)");

    a.parse_check(argc, argv);

    // 获取输入的参数值
    {
        unsigned int addr, ep;
        sscanf(a.get<string>("addr").c_str(), "%x", &addr);
        header.ih_load = BigtoLittle32(addr);
        sscanf(a.get<string>("ep").c_str(), "%x", &ep);
        header.ih_ep = BigtoLittle32(ep);
        header.ih_os = get_value_os(a.get<string>("os"));
        header.ih_arch = get_value_arch(a.get<string>("arch"));
        header.ih_type = get_value_type(a.get<string>("type"));
        header.ih_comp = get_value_comp(a.get<string>("comp"));
        memset(header.ih_name, 0, IH_NMLEN);
        strcpy((char*) header.ih_name, a.get<string>("name").c_str());
        mkuimage(a.get<string>("input").c_str(), a.get<string>("output").c_str());
    }

    return 0;
}

