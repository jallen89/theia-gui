/*
 * I:   %x = alloca i32, align 4
 * I:   %y = alloca i32, align 4
 * I:   %z = alloca i32, align 4
 * I:   %f = alloca i32, align 4
 * I:   %hello = alloca i8*, align 8
 * I:   store i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str, i32 0, i32
 * 0), i8** %hello, align 8
 * I:   store i32 4, i32* %x, align 4
 * I:   store i32 5, i32* %y, align 4
 * I:   %1 = load i32, i32* %x, align 4
 * I:   %2 = load i32, i32* %y, align 4
 * I:   %3 = add nsw i32 %1, %2
 * I:   store i32 %3, i32* %z, align 4
 * I:   %4 = load i32, i32* %z, align 4
 * I:   %5 = load i32, i32* %x, align 4
 * I:   %6 = add nsw i32 %4, %5
 * I:   %7 = load i32, i32* %y, align 4
 * I:   %8 = add nsw i32 %6, %7
 * I:   store i32 %8, i32* %f, align 4
 * I:   ret i32 0
 */


//int func_add(int x1, int y1, int z1) {
//    int z;
//    z = x1 + y1;
//    return z;
//}
//
//
#define IDENTIFIER __attribute__((annotate("identifier")))
#define INDICATOR __attribute__((annotate("indicator")))
#define DELEGATOR __attribute__((annotate("delegator")))
struct test {
    int x;
    int y;
    IDENTIFIER int z;
};

int main() {

    INDICATOR struct test x;
    x.x = 4;
    x.y = 5;
    x.z = 6;
}
