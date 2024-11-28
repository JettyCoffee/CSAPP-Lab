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
定义`Cache_line`结构组：
```0
typedef struct{
    int valid; //有效位
    int tag; //标记位
    int time_stamp; //时间戳
}Cache_line;
```
`valid`和`tag`即为`Cacheline`里的基本参数，`time_stamp`是为了实现LRU策略，为每个`Cache_line`记录其操作时间。  
随后定义`Cache`结构组：

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
初始的`time_stamp`需要全部设置为0，其余具体的代码解释均已写在注释内~  
### LRU策略的实现
LRU（Least Rencently Used）策略是当set满时，自动替换最早使用的cache，对应替换`max_time_stamp`对应的cache。  
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
接下来要解决的问题是，在已知`set_index`与`index`后，如何判断cache是`hit，miss，eviction`？  
先解决`hit or miss`：

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
遍历所有行，检查`valid`是否为1与`tag`是否相等。若相等则返回对应行号；反之则返回-1表示miss  
此时的miss有两种可能性：  
1.`cold miss`：组中有空行，只需要找到valid == 0的行即可  
2.`set已满`：此时需要调用LRU策略进行驱逐替换  
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
通过`is_full`判断，若未满，则返回第一个空行的行号。  
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
其中operation很好获取，address中包含着`tag, set_index, block_offset`三部分  
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
使用`getopt()`函数来获取命令行参数的字符串形式，然后用`atoi()`转换为要用的参数，最后用switch语句跳转到对应功能块。
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
运行make编译csim.c，然后执行`./test-csim`得到成绩：

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
## Part B - Optimizing Matrix Transpose
Part B 是在trans.c中编写矩阵转置的函数，在一个 s = 5, E = 1, b = 5 的缓存中进行读写，使得 miss 的次数最少。测试矩阵的参数以及 miss 次数对应的分数如下：
>  • 32×32: 8points if m < 300, 0 points if m > 600  
>  • 64×64: 8points if m < 1,300, 0 points if m > 2,000  
>  • 61×67: 10points if m < 2,000, 0 points if m > 3,000

要求最多只能声明12个本地变量。

根据课本以及 PPT 的提示，这里肯定要使用矩阵分块进行优化
### 32 × 32
`s = 5, E = 1, b = 5 `的缓存有32组，每组一行，每行存 8 个`int`。

而缓存只够存储一个矩阵的四分之一，A中的元素对应的缓存行每隔8行就会重复。A和B的地址由于取余关系，每个元素对应的地址是相同的。因此如果简单采用暴力算法，对A逐行读取而后按列的方式放入B时，就会出现缓存内数据利用率低，频繁出现miss，驱逐的现象。因此不妨考虑分块矩阵转置的方法。

由转置的性质和缓存大小可以推出，采用8x8的分块较为合适，这样块内的数据不会发生驱逐，接下来考虑AB之间是否会发生内存地址冲突？

前面已经知道AB内存地址相同，那么简单推导发现只有对角线经过的块会发生冲突，考虑用八个本地变量来存储A中这些块内每一行的元素，然后再复制给B。于是代码得到如下：

```0
void transpose_32x32(int M, int N, int A[N][M], int B[M][N])
{
    for (int i = 0; i < 32; i += 8)
        for (int j = 0; j < 32; j += 8)
            for (int k = i; k < i + 8; k++)
            {
                int a_0 = A[k][j];
                int a_1 = A[k][j + 1];
                int a_2 = A[k][j + 2];
                int a_3 = A[k][j + 3];
                int a_4 = A[k][j + 4];
                int a_5 = A[k][j + 5];
                int a_6 = A[k][j + 6];
                int a_7 = A[k][j + 7];
                B[j][k] = a_0;
                B[j + 1][k] = a_1;
                B[j + 2][k] = a_2;
                B[j + 3][k] = a_3;
                B[j + 4][k] = a_4;
                B[j + 5][k] = a_5;
                B[j + 6][k] = a_6;
                B[j + 7][k] = a_7;
            }
}
```
运行Test结果如下（`miss = 288`满足要求）：

```0
root@OyamaMahiro:~/CSAPP-Lab/Cache_Lab# ./test-trans -M 32 -N 32

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:1765, misses:288, evictions:256

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:869, misses:1184, evictions:1152

Summary for official submission (func 0): correctness=1 misses=288

TEST_TRANS_RESULTS=1:288
```
### 64 x 64
同理，矩阵每四行就会占满一个缓存，相同方法需要`4x4`的分块，但尝试结果并未满分，只好回到`8x8`想一想有没有解决办法。

思来想去，根据缓存已保存矩阵四行的结果，可以考虑先一次性将A的左上和右上共计`4x8`复制给B，而后将B的右上保存在本地变量，再将A的左下复制到B的右上，从本地变量复制到B的左下，最后将A的右下复制到B的右下。这样能保证在不发生冲突的前提下，充分利用缓存的内容。

具体代码如下（代码编写时需要尤其注意`i，j，k`对应的是行还是列）：

```0
void transpose_64x64(int M, int N, int A[N][M], int B[M][N])
{
    int a_0, a_1, a_2, a_3, a_4, a_5, a_6, a_7;
    for(int i = 0; i < N; i += 8){
        for(int j = 0; j < M; j += 8){
            //利用现有缓存将A的左上(4x4)复制到B的左上
            for(int k = i; k < i + 4; k++){
                a_0 = A[k][j];
                a_1 = A[k][j + 1];
                a_2 = A[k][j + 2];
                a_3 = A[k][j + 3];
                a_4 = A[k][j + 4];
                a_5 = A[k][j + 5];
                a_6 = A[k][j + 6];
                a_7 = A[k][j + 7];
                B[j][k] = a_0;
                B[j + 1][k] = a_1;
                B[j + 2][k] = a_2;
                B[j + 3][k] = a_3;
                B[j][k + 4] = a_4;
                B[j + 1][k + 4] = a_5;
                B[j + 2][k + 4] = a_6;
                B[j + 3][k + 4] = a_7; 
            }
            //将B的右上(4x4)存入临时变量，再将A的左下(4x4)存入B的右上
            for(int k = j; k < j + 4; k++){
                //得到B的右上
                a_4 = B[k][i + 4];
                a_5 = B[k][i + 5];
                a_6 = B[k][i + 6];
                a_7 = B[k][i + 7];
                //得到A的左下
                a_0 = A[i + 4][k];
                a_1 = A[i + 5][k];
                a_2 = A[i + 6][k];
                a_3 = A[i + 7][k];
                //存入B的右上
                B[k][i + 4] = a_0;
                B[k][i + 5] = a_1;
                B[k][i + 6] = a_2;
                B[k][i + 7] = a_3;
                //存入B的左下
                B[k + 4][i] = a_4;
                B[k + 4][i + 1] = a_5;
                B[k + 4][i + 2] = a_6;
                B[k + 4][i + 3] = a_7;
            }
            //转移A的右下(4x4)到B的右下
            for(int k = i + 4; k < i + 8; k++){
                a_0 = A[k][j + 4];
                a_1 = A[k][j + 5];
                a_2 = A[k][j + 6];
                a_3 = A[k][j + 7];
                B[j + 4][k] = a_0;
                B[j + 5][k] = a_1;
                B[j + 6][k] = a_2;
                B[j + 7][k] = a_3;
            }
        }
    }
}
```
运行结果（`miss = 1228`满足要求）：

```0
root@OyamaMahiro:~/CSAPP-Lab/Cache_Lab# ./test-trans -M 64 -N 64

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:9017, misses:1228, evictions:1196

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:3473, misses:4724, evictions:4692

Summary for official submission (func 0): correctness=1 misses=1228

TEST_TRANS_RESULTS=1:1228
```
### 61 × 67
由于`miss`的限制很小，所以不妨把分块改为`16x16`试试：

```0
void transpose_61x67(int M, int N, int A[N][M], int B[M][N]){
    for (int i = 0; i < N; i += 16)
        for (int j = 0; j < M; j += 16)
            for (int k = i; k < i + 16 && k < N; k++)
                for (int s = j; s < j + 16 && s < M; s++)
                    B[s][k] = A[k][s];
}
```
运行结果刚好满足要求（`miss = 1993`）：

```0
root@OyamaMahiro:~/CSAPP-Lab/Cache_Lab# ./test-trans -M 61 -N 67

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:6186, misses:1993, evictions:1961

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:3755, misses:4424, evictions:4392

Summary for official submission (func 0): correctness=1 misses=1993

TEST_TRANS_RESULTS=1:1993
```
## 总结
Part A 让我对缓存的设计有了更深入的理解，其中替换策略也值得以后继续研究

Part B 为我展示了计算机之美，一个简简单单的转置函数，无论怎么写，时间复杂度都是`O(n^2)`，然而因为缓冲区的问题，不同代码的性能竟然有着天壤之别。编写函数过程中，对miss的估量与计算很烧脑，但也很有趣。