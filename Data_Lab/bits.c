/* 
 * CS:APP Data Lab 
 * 
 * 10235501454 陈子谦
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  return ~( x & y ) &  ~( ~x & ~y );
}
//通过德摩根定律，将异或运算转换为与和非运算
//
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1 << 31;
}
//通过1左移31位得到最小的补码数
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  //First solution:!((x + 1) ^ (~x) | !(x+1));
  return !((x+x+2) | !(x+1));
}
//因为x=Tmax时，x + (x + 1) = -1, 所以 2x+2 = 0，同时为了排除x=-1的情况，所以需要加上一个判断
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  int mask = 0xAA;
  mask = mask | (mask << 8);
  mask = mask | (mask << 16);
  return !( (x&mask) ^ mask);
}
//通过构造一个mask，然后与x进行与运算，然后与mask进行异或运算，若x与mask相同则返回1，否则返回0
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1;
}
//通过取反加一的方式实现取反（负数的补码等于其取反加一）
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  int min = ~0x30 + 1;
  int max = ~0x3A + 1;
  return (!((x + min) >> 31)) & ((x + max) >> 31);
}
//因为求x是否在范围内，实际在求x-0x30和x-0x3A的符号是否不同，括号内若相同则结果为0，不同则结果为1
//计算x-0x30和x-0x3A的符号，然后通过与运算判断是否为0
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int mask = ~!x + 1;
  return (~mask & y) | (mask & z);
}
//通过将x转换成掩码Ox00000000或者OxFFFFFFFF，然后通过与操作得到结果
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int x_sign = x >> 31 & 1;
  int y_sign = y >> 31 & 1;
  int y_minus_x = y + ~x + 1;
  return (x_sign & !y_sign) | (!(x_sign ^ y_sign) & !(y_minus_x >> 31));
}
//首先判断x和y的符号，然后计算y-x的值
//若x为负，y为正，则因为或运算返回1
//若x和y符号相同，则判断y-x的符号是否为正，若为正则返回1
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  return ((x | (~x + 1)) >> 31) + 1;
}
//为了实现逻辑取反，可以通过将x与其取反加一的或运算，然后右移31位，再加一，即可得到逻辑取反的结果
//其中的x与其取反加一的或运算，可以得到x为0时的结果为0，x不为0时的结果为-1
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */

int howManyBits(int x) {
  int sign = x >> 31;
  int high16, high8, high4, high2, high1, high0;
  x = (sign & ~x) | (~sign & x);
  high16 = !!(x >> 16) << 4;
  x = x >> high16;
  high8 = !!(x >> 8) << 3;
  x = x >> high8;
  high4 = !!(x >> 4) << 2;
  x = x >> high4;
  high2 = !!(x >> 2) << 1;
  x = x >> high2;
  high1 = !!(x >> 1);
  x = x >> high1;
  high0 = x;
  return high16 + high8 + high4 + high2 + high1 + high0 + 1;
}
//对于输出，可以发现对于正数，其比无符号正数最高位多一位0,；对于负数，其比无符号正数最高位多一位1
//因此可以先将负数转化为无符号数，然后再计算其位数，最后加上1即可
//通过逐次右移，判断其前high x高位是否有1，若有则将其加入到结果中，逐项递归后得到结果。
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  unsigned exp = (uf >> 23) & 0xFF;
  unsigned sign = uf & 0x80000000;
  unsigned frac = uf & 0x7FFFFF;
  //for NaN
  if (exp == 0xFF) {
    return uf;
  }
  //for denormalized number
  if (exp == 0){
    frac = frac << 1;
    return sign | frac;
  }
  //for normalized number
  exp = exp + 1;
  //for overflow
  if (exp == 0xFF) {
    frac = 0;
  }
  return sign | (exp << 23) | frac;
}
//首先分别取出符号位，指数位和尾数位
//对于NaN，由题目要求直接返回
//对于非规格化数，x2即为尾数左移一位
//对于规格化数，指数加一即可
//对于指数溢出的情况，尾数直接置零即可，此时指数为0xFF即为INF
//最后将符号位，指数位和尾数位合并即可
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  unsigned exp = (uf >> 23) & 0xFF;
  unsigned sign = uf >> 31;
  unsigned frac = uf & 0x7FFFFF;
  int E = exp - 127;
  if (E < 0) {
    return 0;
  }
  if (E > 31) {
    return 0x80000000u;
  }
  frac = frac | 0x800000;
  if (E > 23) {
    frac = frac << (E - 23);
  } else {
    frac = frac >> (23 - E);
  }
  if (frac >> 31) {
    return 0x80000000u;
  }
  return sign ? -frac : frac;
}
//首先分别取出符号位，指数位和尾数位
//由于bias为127，所以指数减去127即为E，E小于0时因为取整向下取整，所以直接返回0
//E大于31时，因为超出int范围，所以返回0x80000000u（此时就可以不需要考虑溢出的情况因为已经包含在内了）
//对于E大于23的情况，尾数左移E-23位即可，同理有E小于23的情况
//最后需要判断是否溢出，若溢出则返回0x80000000u
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  if (x < -126) {
    return 0;
  }
  if (x > 127) {
    return 0x7F800000;
  }
  return (x + 127) << 23;
}
//聚焦于x的范围即可，float的范围为-126到127，对于小于-126的情况，返回0即可