# Cache Lab - JettyCoffee
## Part A - Writing a Cache Simulator
Part A 要求在`csim.c`下编写一个缓存模拟器，有以下六个参数输入（本题内我们只考虑实现s,E,b,t）：

```0
Usage: ./csim-ref [-hv]-s <s>-E <E>-b <b>-t <tracefile>
 •-h: Optional help flag that prints usage info
 •-v: Optional verbose flag that displays trace info
 •-s <s>: Number ofset index bits (S = 2s is the number of sets)
 •-E <E>: Associativity (number of lines per set)
 •-b <b>: Number ofblock bits (B = 2b is the block size)
 •-t <tracefile>:Nameof the valgrindtrace to replay
```
其中，输入的 trace 的格式为：`[space]operation address, size，operation` 有 4 种：

> I 加载指令  
> L 加载数据  
> S 存储数据  
> M 修改数据

模拟器不需要考虑加载指令，而M指令就相当于先进行L再进行S，因此，要考虑的情况其实并不多。模拟器要做出的反馈有 3 种：

> hit：命中，表示要操作的数据在对应组的其中一行  
> miss：不命中，表示要操作的数据不在对应组的任何一行  
> eviction：驱赶，表示要操作的数据的对应组已满，进行替换操作

准备工作结束，下面开始编写`csim.c`！  
### 数据结构
定义Cache_line结构组：
```0
typedef struct{
    int valid; //有效位
    int tag; //标记位
    int time_stamp; //时间戳
}Cache_line;
```
valid和tag即为cacheline里的基本参数，time_stamp是为了实现LRU策略，为每个cache_line记录其操作时间。  
随后定义Cache结构组：

```0
typedef struct{
    int S; //组数
    int E; //每组行数
    int B; //块大小
    Cache_line **line;（用二维指针建立cache索引）
}Cache;
```
用该结构组表示一个缓存，根据CMU教程的提示可以知道Cache近似一个二维数组，数组内的一个元素就是缓存本身。  
### 初始化Cache
```0
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
```
初始的time_stamp需要全部设置为0，其余具体的代码解释均已写在注释内~  
### LRU策略的实现
LRU（Least Rencently Used）策略是当set满时，自动替换最早使用的cache，对应替换max_time_stamp对应的cache。  
首先先确定ops更新的代码：
```0
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
```
这段代码在找到要进行的操作行后调用（无论是不命中还是命中，还是驱逐后）。前两行是对有效位和标志位的设置，与时间戳无关，主要关注后几行：  
遍历组中每一行，并将它们的值加1，也就是说每一行在进行一次操作后时间戳都会变大，表示它离最后操作的时间变久  
将本次操作的行时间戳设置为最小，也就是0  
由此，每次只需要找到时间戳最大的行进行替换就可以了：
```0
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
```
### 缓存查找与更新
接下来要解决的问题是，在已知set_index与index后，如何判断cache是hit，miss，eviction？  
先解决hit or miss：

```0
int get_index(int set_index, int tag){
    for(int i = 0; i < cache->E; i++){
        if(cache->line[set_index][i].valid && cache->line[set_index][i].tag == tag){
            return i; //返回命中的行号
        }
    }
    return -1; //返回未命中
}
```
遍历所有行，检查valid值是否为1与tag是否相等。若相等则返回对应行号；反之则返回-1表示miss  
此时的miss有两种可能性：  
1.cold miss：组中有空行，只需要找到valid == 0的行即可  
2.set已满：此时需要调用LRU策略进行驱逐替换  
```0
int is_full(int set_index){
    for(int i = 0; i < cache->E; i++){
        if(cache->line[set_index][i].valid == 0){
            return i;
        }
    }
    return -1;
}
```
通过is_full判断，若未满，则返回第一个空行的行号。  
检查状态完成后，就可以组织对cache的访问函数了：

```0
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
```
至此就已经实现完毕Cache的模拟器功能了，下面需要编写的是如何将终端的参数传入和trace的参数调用功能。
### Trace内容解析
trace 的格式为：`[space]operation address, size，operation`  
其中operation很好获取，address中包含着tag, set_index, block_offset三部分  
分别通过移位操作得到tag和set_index;  
`int set_index = (address >> b) & ((unsigned)(-1) >> (8 * sizeof(unsigned) - s));`  
`int tag = address >> (s + b);`  
其中`((unsigned)(-1) >> (8 * sizeof(unsigned) - s))`是为了构建一个s位的掩码以获取set_index
```0
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
```
### 命令行参数获取
使用getopt()函数来获取命令行参数的字符串形式，然后用atoi()转换为要用的参数，最后用switch语句跳转到对应功能块。
```0
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
```
需要注意的是，所有的参数都在结构体后直接定义以方便调取：

```0
int h,v,s,E,b; //命令行参数
int hit, miss, eviction; //命中，未命中，替换次数
char t[1000]; //trace文件
Cache *cache; //cache
```
运行make编译csim.c，然后执行./test-csim得到成绩：

```0
root@OyamaMahiro:/home/jettycoffee/cachelab-handout# ./test-csim
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27

TEST_CSIM_RESULTS=27
```