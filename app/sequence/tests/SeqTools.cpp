
#include <stdio.h>
#include <string>
#include<stdlib.h>
#include <sstream>  
#include <iostream>
#include <stdint.h>

/** 开始时间截 (2018-01-01) */
const static int64_t twepoch = 1514764800000;

/** 机器id所占的位数 */
const static unsigned int workerIdBits = 5;

/** 数据标识id所占的位数 */
const static unsigned int datacenterIdBits = 5;

/** 支持的最大机器id，结果是31 (这个移位算法可以很快的计算出几位二进制数所能表示的最大十进制数) */
static unsigned int maxWorkerId = -1 ^ (-1 << workerIdBits);

/** 支持的最大数据标识id，结果是31 */
const static unsigned int maxDatacenterId = -1 ^ (-1 << datacenterIdBits);

/** 序列在id中占的位数 */
const static unsigned int sequenceBits = 12;

/** 机器ID向左移12位 */
const static unsigned int workerIdShift = sequenceBits;

/** 数据标识id向左移17位(12+5) */
const static unsigned int datacenterIdShift = sequenceBits + workerIdBits;

/** 时间截向左移22位(5+5+12) */
const static unsigned int timestampLeftShift = sequenceBits + workerIdBits + datacenterIdBits;

/** 生成序列的掩码，这里为4095 (0b111111111111=0xfff=4095) */
const static unsigned int sequenceMask = -1 ^ (-1 << sequenceBits);

/** 32位序列 - 数据标识id所占的位数,可以部署在32个节点 */
static unsigned int datacenterIdBits32 = 5L;

/** 64位序列 - 数据标识id所占的位数,可以部署在1024个节点*/
static unsigned int datacenterIdBits64 = 10L;

/** 32位序列 - 序列在id中占的位数 */
const static unsigned int sequenceBits32 = 31 - datacenterIdBits32;

/** 64位序列 - 序列在id中占的位数 */
const static unsigned int sequenceBits64 = 61 - datacenterIdBits64;

#define INCREMENT 0
#define INCREMENT64 2
#define INCREMENT32 1
#define SNOW_FLAKE 3

int main(int argc, char** argv) {
    /** 序列 */
    int64_t sequence;
    /** 毫秒内序列(0~4095) */
    unsigned int msec_sequence;
    /** 生成ID的时间截 */
    int64_t timestamp;
    /** 工作机器ID(0~31) */
    unsigned int workerId;
    /** 数据中心ID(0~31) */
    int datacenterId;

    if(argc<3){
        std::cout << "Useage: ./tools sequence algorithm" <<std::endl;
        return -1;
    }
    sequence = atol(argv[1]);
    int algorithm = atoi(argv[2]);
    switch(algorithm){
        case INCREMENT64:
        datacenterId = sequence >> sequenceBits64 & (-1 ^ (-1 << datacenterIdBits64));
        sequence = (-1 ^ (-1 << sequenceBits64)) & sequence;
        std::cout << "datacenterId:" << datacenterId << ", sequence:" << sequence << std::endl;
        break;
        case INCREMENT32:
        datacenterId = sequence >> sequenceBits32 & (-1 ^ (-1 << datacenterIdBits32));
        sequence = (-1 ^ (-1 << sequenceBits32)) & sequence;
        std::cout << "datacenterId:" << datacenterId << ", sequence:" << sequence << std::endl;
        break;
        case SNOW_FLAKE:
        timestamp = (sequence>>timestampLeftShift) + twepoch;
        workerId = sequence>> sequenceBits & (-1 ^ (-1 << 10));
        sequence = sequenceMask & sequence;
        std::cout << "timestamp:" << timestamp << ", workerId:" << workerId << ", sequence:" << sequence << std::endl;
        break;
        default:
        break;
    }
    return 0;
}
