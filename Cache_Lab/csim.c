#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

typedef struct{
    int valid; //有效位
    int tag; //标记位
    int time_stamp; //时间戳
}Cache_line;

typedef struct{
    int S; //组数
    int E; //每组行数
    int B; //块大小
    Cache_line **line; //行指针（用二维指针建立cache索引）
}Cache;

int h,v,s,E,b; //命令行参数
int hit, miss, eviction; //命中，未命中，替换次数
char t[1000]; //trace文件
Cache *cache; //cache


//初始化cache
void init_cache(int s, int E, int b){
    int S = 1 << s; //组数(2^s)
    int B = 1 << b; //块大小(2^b)
    cache = (Cache *)malloc(sizeof(Cache)); //分配cache空间
    cache->S = S;
    cache->E = E;
    cache->B = B;
    cache->line = (Cache_line **)malloc(S * sizeof(Cache_line *)); //分配组数的行指针
    for(int i = 0; i < S; i++){ //遍历每组
        cache->line[i] = (Cache_line *)malloc(E * sizeof(Cache_line)); //分配每组的行数
        for(int j = 0; j < E; j++){
            cache->line[i][j].valid = 0;
            cache->line[i][j].tag = 0;
            cache->line[i][j].time_stamp = 0;
        }
    }
}

//释放cache
void free_cache(){
    for(int i = 0; i < cache->S; i++){
        free(cache->line[i]);
    }
    free(cache->line);
    free(cache);
}

//update cache's stamp
void update(int set_index, int index, int tag){
    cache->line[set_index][index].valid = 1;
    cache->line[set_index][index].tag = tag;
    for(int i = 0; i < cache->E; i++){
        if(cache->line[set_index][i].valid == 1){
            cache->line[set_index][i].time_stamp++;
        }
    }
    cache->line[set_index][index].time_stamp = 0;
}

//LRU替换策略
int lru_replace(int set_index){
    int max_time_stamp = 0;
    int max_time_stamp_index = 0;
    for(int i = 0; i < cache->E; i++){
        if(cache->line[set_index][i].time_stamp > max_time_stamp){
            max_time_stamp = cache->line[set_index][i].time_stamp; //找到最大时间戳
            max_time_stamp_index = i; //记录最大时间戳的行号
        }
    }
    return max_time_stamp_index;//返回最大时间戳的行号
}

//访问cache返回index
int get_index(int set_index, int tag){
    for(int i = 0; i < cache->E; i++){
        if(cache->line[set_index][i].valid && cache->line[set_index][i].tag == tag){
            return i; //返回命中的行号
        }
    }
    return -1; //返回未命中
}

//cacheline是否为满
int is_full(int set_index){
    for(int i = 0; i < cache->E; i++){
        if(cache->line[set_index][i].valid == 0){
            return i;
        }
    }
    return -1;
}

//访问cache(命中，未命中，替换)
void access_cache(int set_index, int tag){
    //首先判断是否命中
    int index = get_index(set_index, tag);
    if (index == -1){ //未命中
        miss += 1;
        int index = is_full(set_index); //判断cache是否满
        if(index == -1){ //cache满
            eviction += 1;
            index = lru_replace(set_index); //LRU替换
        }
        update(set_index, index, tag); //更新cache
    }
    else{
        hit += 1;
        update(set_index, index, tag); //更新cache
    }
}

//get_trace
void get_trace(){
    FILE *fp;
    fp = fopen(t, "r");
    if(fp == NULL){
        printf("open file failed\n");
        exit(1);
    }
    char identifier;
    unsigned address;
    int size;
    while(fscanf(fp, " %c %x,%d", &identifier, &address, &size) > 0){
        if(identifier == 'I'){
            continue;
        }
        if(identifier == 'L' || identifier == 'S' || identifier == 'M'){
            int set_index = (address >> b) & ((unsigned)(-1) >> (8 * sizeof(unsigned) - s)); //取出组索引
            int tag = address >> (s + b); //取出标记
            if(identifier == 'M'){
                access_cache(set_index, tag); //访问cache
            }
            access_cache(set_index, tag); //访问cache
        }
    }
}

int main(int argc, char *argv[])//命令行参数
{
    char opt;
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1){
        switch(opt){
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                strcpy(t, optarg);
                break;
            default:
                break;
        }
    }
    init_cache(s, E, b);
    get_trace();
    free_cache();
    printSummary(hit, miss, eviction);
    return 0;
}
