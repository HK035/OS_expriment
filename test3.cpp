/***************************************************************
 *  实验三  页面置换算法的模拟实现
 *
 *  模拟请求分页存储管理中的页面置换过程，实现：
 *      1) FIFO —— 先进先出算法
 *      2) LRU  —— 最近最久未使用算法
 *      3) OPT  —— 最佳淘汰算法
 *
 *  程序随机产生页面走向，输出每次访问后的内存块状态，
 *  并统计缺页次数与缺页率。
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define L 20        // 页面走向的最大长度
#define M 5         // 内存块的最大数目

//------------------------- 页面结构 ---------------------------
struct Pro
{
    int pn;         // 页面号，0 表示该内存块空闲
    int time;       // 页面未被使用的时间，供 LRU 使用
};

//------------------------- 全局变量 ---------------------------
Pro page[M];        // 内存块
Pro p[L];           // 页面走向
int m = 4;          // 可用内存页面数
int l = 18;         // 实际页面走向长度

//------------------------- 函数声明 ---------------------------
void Input(int m, Pro p[L]);                        // 产生并输出页面走向
void print(Pro *page1);                             // 输出当前内存块状态
int  Search(int e, Pro *page1);                     // 查找页面 e 所在的内存块号
int  Max(Pro *page1);                               // 寻找最近最久未使用的页面
int  Count(Pro *page1, int i, int t, Pro p[L]);     // 页面 t 距下次被访问的间隔
void FIFO(int m, Pro p[L]);                         // 先进先出页面置换
void LRU(int m, Pro p[L]);                          // 最近最久未使用页面置换
void OPT(int m, Pro p[L]);                          // 最佳淘汰页面置换

//-------------------------------------------------------------
// 产生页面走向并输出随机数，同时把内存块初始化为空闲
void Input(int m, Pro p[L])
{
    int i;

    printf("输出随机数：");
    for (i = 0; i < l; i++)
    {
        p[i].pn = rand() % 10 + 1;   // 页面号范围为 1～10
        p[i].time = 0;
        printf(" %d", p[i].pn);
    }
    printf("\n");

    for (i = 0; i < m; i++)
    {
        page[i].pn = 0;
        page[i].time = 0;
    }
}

//-------------------------------------------------------------
// 输出当前内存块状态，空闲内存块输出一个空白字段
void print(Pro *page1)
{
    int i;

    for (i = 0; i < m; i++)
    {
        if (page1[i].pn != 0)
            printf("%d\t", page1[i].pn);
        else
            printf("\t");
    }
}

//-------------------------------------------------------------
// 在内存块中查找页面 e，找到返回块号，否则返回 -1
int Search(int e, Pro *page1)
{
    int i;

    for (i = 0; i < m; i++)
    {
        if (page1[i].pn == e)
            return i;
    }
    return -1;
}

//-------------------------------------------------------------
// 寻找最近最久未使用的页面（time 值最大的内存块）
int Max(Pro *page1)
{
    int i;
    int k = 0;

    for (i = 1; i < m; i++)
    {
        if (page1[i].time > page1[k].time)
            k = i;
    }
    return k;
}

//-------------------------------------------------------------
// 计算内存块 t 中的页面在第 i 次访问之后，距下次被访问的间隔；
// 若以后不再使用，则返回一个比任何实际间隔都大的值
int Count(Pro *page1, int i, int t, Pro p[L])
{
    int j;

    for (j = i + 1; j < l; j++)
    {
        if (page1[t].pn == p[j].pn)
            return j - i;
    }
    return l - i + 1;
}

//-------------------------------------------------------------
// 1) 先进先出算法 FIFO：淘汰最先装入内存的页面
void FIFO(int m, Pro p[L])
{
    int i;
    int t;
    int faults = 0;     // 缺页次数
    int filled = 0;     // 已占用的内存块数
    int pos = 0;        // 下一个被淘汰的内存块号

    for (i = 0; i < m; i++)
    {
        page[i].pn = 0;
        page[i].time = 0;
    }

    printf("\n\t\tFIFO算法置换情况如下:\n");
    printf("************************************************************\n");

    for (i = 0; i < l; i++)
    {
        int e = p[i].pn;

        t = Search(e, page);
        if (t != -1)                    // 页面已在内存中，未缺页
        {
            printf("%d\t不缺页\n", e);
            continue;
        }

        faults++;
        if (filled < m)                 // 还有空闲内存块，直接装入
        {
            page[filled].pn = e;
            filled++;
        }
        else                            // 内存已满，淘汰最先装入的页面
        {
            page[pos].pn = e;
            pos = (pos + 1) % m;
        }

        printf("%d\t", e);
        print(page);
        printf("√\n");
    }

    printf("缺页次数：%d    缺页率：%f\n", faults, (double)faults / l);
}

//-------------------------------------------------------------
// 2) 最近最久未使用算法 LRU：淘汰最近一段时间内最久未被访问的页面
void LRU(int m, Pro p[L])
{
    int i, j;
    int t;
    int faults = 0;
    int filled = 0;

    for (i = 0; i < m; i++)
    {
        page[i].pn = 0;
        page[i].time = 0;
    }

    printf("\n\t\tLRU算法置换情况如下:\n");
    printf("************************************************************\n");

    for (i = 0; i < l; i++)
    {
        int e = p[i].pn;

        for (j = 0; j < m; j++)         // 每访问一次，未使用时间加 1
        {
            if (page[j].pn != 0)
                page[j].time++;
        }

        t = Search(e, page);
        if (t != -1)                    // 页面已在内存中，未缺页
        {
            page[t].time = 0;           // 该页面刚被访问过
            printf("%d\t不缺页\n", e);
            continue;
        }

        faults++;
        if (filled < m)                 // 还有空闲内存块，直接装入
        {
            page[filled].pn = e;
            page[filled].time = 0;
            filled++;
        }
        else                            // 内存已满，淘汰最久未使用的页面
        {
            t = Max(page);
            page[t].pn = e;
            page[t].time = 0;
        }

        printf("%d\t", e);
        print(page);
        printf("√\n");
    }

    printf("缺页次数：%d    缺页率：%f\n", faults, (double)faults / l);
}

//-------------------------------------------------------------
// 3) 最佳淘汰算法 OPT：淘汰以后永不使用或最长时间内不再被访问的页面
void OPT(int m, Pro p[L])
{
    int i, j;
    int t, k;
    int faults = 0;
    int filled = 0;
    int max, c;

    for (i = 0; i < m; i++)
    {
        page[i].pn = 0;
        page[i].time = 0;
    }

    printf("\n\t\tOPT算法置换情况如下:\n");
    printf("************************************************************\n");

    for (i = 0; i < l; i++)
    {
        int e = p[i].pn;

        t = Search(e, page);
        if (t != -1)                    // 页面已在内存中，未缺页
        {
            printf("%d\t不缺页\n", e);
            continue;
        }

        faults++;
        if (filled < m)                 // 还有空闲内存块，直接装入
        {
            page[filled].pn = e;
            filled++;
        }
        else                            // 内存已满，淘汰未来最久才使用的页面
        {
            k = 0;
            max = Count(page, i, 0, p);
            for (j = 1; j < m; j++)
            {
                c = Count(page, i, j, p);
                if (c > max)
                {
                    max = c;
                    k = j;
                }
            }
            page[k].pn = e;
        }

        printf("%d\t", e);
        print(page);
        printf("√\n");
    }

    printf("缺页次数：%d    缺页率：%f\n", faults, (double)faults / l);
}

//-------------------------------------------------------------
// 主函数
int main()
{
    int choice;

    // 为便于复现实验结果，这里使用固定种子；
    // 若需要每次运行产生不同的随机序列，可改为 srand((unsigned)time(NULL))
    srand(1324972787);

    printf("\t\t*********************************************\n");
    printf("\t\t*              页式存储管理                 *\n");
    printf("\t\t*********************************************\n");

    printf("请输入可用内存页面数m(3～5): ");
    scanf("%d", &m);
    if (m < 3 || m > 5)
        m = 4;

    printf("请输入实际页面走向长度L(15<=L<=20): ");
    scanf("%d", &l);
    if (l < 15 || l > 20)
        l = 18;

    Input(m, p);

    printf("^_^欢迎进入操作系统界面^_^\n");
    printf("1:FIFO页面置换\n");
    printf("2:LRU页面置换\n");
    printf("3:OPT页面置换\n");
    printf("4:退出\n");

    while (1)
    {
        printf("按1～4键操作:");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            FIFO(m, p);
            break;
        case 2:
            LRU(m, p);
            break;
        case 3:
            OPT(m, p);
            break;
        case 4:
            printf("^_^再见! ^_^\n");
            return 0;
        default:
            printf("输入有误，请重新输入！\n");
            break;
        }
    }

    return 0;
}
