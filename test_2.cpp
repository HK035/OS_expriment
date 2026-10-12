/***************************************************************
 *  实验二  进程调度算法的模拟实现
 *
 *  模拟单处理机环境下的处理机调度，实现：
 *      1) FCFS  —— 先来先服务算法
 *      2) RR    —— 时间片轮转算法
 *      3) PRIOR —— 优先级调度算法
 *      4) SJF   —— 最短作业（进程）优先算法
 *      5) SRTF  —— 最短剩余时间优先算法（抢占式）
 *
 *  并进行算法评价：计算平均等待时间和平均周转时间。
 *
 *  说明：
 *      FCFS、RR、PRIOR、SJF 四个算法都以数组（PCB 数组）描述进程，
 *      只有 SRTF 用指针来描述进程（见 void srtf()）。
 ***************************************************************/

#include <iostream>
#include <cstdio>
#include <cstring>
#include <climits>

using namespace std;

#define N 10       // 最多进程数
#define FREE 0     // 未到达 / 空闲标志
#define RUN 1      // 就绪或运行标志
#define FINISH 2   // 完成标志

//----------------------- 进程控制块 PCB -----------------------
struct PCB
{
    char name[10];  // 进程名字
    int time;       // 共需占用 CPU 时间
    int need;       // 还需要占用时间
    int pri;        // 优先级（运行后的优先级）：值越小优先级越高
    int status;     // 状态：FREE 未到达、RUN 就绪/运行、FINISH 完成
    int arrive;     // 到达时间（仅 SRTF 使用）
};

//------------------------- 全局变量 --------------------------
PCB pcbs[N];       // 原始进程信息（每个算法执行前由 reset() 恢复到 prof 中）
PCB prof[N];       // 本次运行所使用的进程副本
int profNum = 0;   // 进程个数
int timeSlice = 0; // 时间片大小
int nowTime = 0;   // 当前时间（SRTF 用）

//----------------------- 函数声明 ----------------------------
int selectAlgorithm();          // 选择调度算法
void inputProcess();            // 输入进程信息
void reset();                   // 恢复进程信息
void showTable(int alg);        // 显示进程状态表
void fncfs();                   // 1) 先来先服务
void rrobin();                  // 2) 时间片轮转
void priorSched();              // 3) 优先级调度
void sjf();                     // 4) 最短作业优先
void srtf();                    // 5) 最短剩余时间优先
void statistics(int order[], int n); // 计算平均等待时间、平均周转时间
void showOrder(int order[], int n);  // 显示进程执行次序

//-------------------------------------------------------------
// 计算字符串的显示宽度（一个汉字按 2 列计算）
int strWidth(const char *s)
{
    int w = 0;
    for (int i = 0; s[i] != '\0';)
    {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80)                  // ASCII 字符
        {
            w += 1;
            i += 1;
        }
        else if ((c & 0xF0) == 0xE0)   // 3 字节：汉字
        {
            w += 2;
            i += 3;
        }
        else                            // 其它多字节字符按 2 列处理
        {
            w += 2;
            i += 2;
        }
    }
    return w;
}

// 把一个字符缓冲区按指定显示宽度左对齐打印
void printW(const char *s, int width)
{
    printf("%s", s);
    int blank = width - strWidth(s);
    while (blank-- > 0)
        putchar(' ');
}

// 把整数按指定显示宽度左对齐打印
void printIntW(int value, int width)
{
    char buf[16];
    sprintf(buf, "%d", value);
    printW(buf, width);
}

//-------------------------------------------------------------
// 显示进程状态表
void showTable(int alg)
{
    // 各列的显示宽度（汉字按 2 列）
    int wName = strWidth("进程名字");   // 8
    int wTime = strWidth("共需占用CPU时间"); // 16
    int wNeed = strWidth("还需要占用时间");  // 14
    int wPri = strWidth("优先级");      // 6
    int wStat = strWidth("状态");       // 4

    for (int i = 0; i < profNum; i++)
    {
        if (strWidth(prof[i].name) > wName) wName = strWidth(prof[i].name);
        char buf[16];
        sprintf(buf, "%d", prof[i].time);
        if ((int)strlen(buf) + 2 > wTime) wTime = (int)strlen(buf) + 2;
        sprintf(buf, "%d", prof[i].need);
        if ((int)strlen(buf) + 2 > wNeed) wNeed = (int)strlen(buf) + 2;
        sprintf(buf, "%d", prof[i].pri);
        if ((int)strlen(buf) + 2 > wPri) wPri = (int)strlen(buf) + 2;
    }
    if (strWidth("就绪") > wStat) wStat = strWidth("就绪"); // "就绪" 比 "状态" 宽

    printW("进程名字", wName);
    printW("共需占用CPU时间", wTime);
    printW("还需要占用时间", wNeed);
    printW("优先级", wPri);
    printf("状态\n");

    for (int i = 0; i < profNum; i++)
    {
        printW(prof[i].name, wName);
        printIntW(prof[i].time, wTime);
        printIntW(prof[i].need, wNeed);
        printIntW(prof[i].status == FINISH ? 0 : prof[i].pri, wPri);

        if (prof[i].status == FINISH)
            printf("完成\n");
        else if (prof[i].need == prof[i].time)
            printf("就绪\n");
        else
            printf("运行\n");
    }
    printf("\n");
}

//-------------------------------------------------------------
// 恢复进程信息：每个算法开始前把原始 PCB 复制到工作副本
void reset()
{
    for (int i = 0; i < profNum; i++)
    {
        prof[i] = pcbs[i];
        prof[i].need = prof[i].time; // 还需要占用时间 = 共需占用时间
        prof[i].status = RUN;        // 初始均处于就绪状态
    }
    nowTime = 0;
}

//-------------------------------------------------------------
// 选择调度算法
int selectAlgorithm()
{
    int alg;
    printf("选择调度算法:\n");
    printf("1: FCFS  2: 时间片轮转  3: 优先级调度  4: 最短作业优先  5: 最短剩余时间优先\n");
    scanf("%d", &alg);
    return alg;
}

//-------------------------------------------------------------
// 输入进程信息
void inputProcess()
{
    printf("输入进程个数:\n");
    scanf("%d", &profNum);
    if (profNum <= 0 || profNum > N)
    {
        printf("进程个数不合法，按 1 处理！\n");
        profNum = 1;
    }

    printf("输入此进程时间片大小:\n");
    scanf("%d", &timeSlice);
    if (timeSlice <= 0)
        timeSlice = 1;

    for (int i = 0; i < profNum; i++)
    {
        printf("输入第 %d 个进程的名字、cpu时间和优先级:\n", i + 1);
        scanf("%s %d %d", pcbs[i].name, &pcbs[i].time, &pcbs[i].pri);
        if (pcbs[i].time <= 0)
            pcbs[i].time = 1;
        pcbs[i].need = pcbs[i].time;
        pcbs[i].status = RUN;
        pcbs[i].arrive = i; // 按输入次序到达
    }

    printf("\n< 说明：在本程序所列进程信息中， 优先级一项是指进程运行后的优先级！！ >\n");
}

//-------------------------------------------------------------
// 显示进程执行次序
void showOrder(int order[], int n)
{
    printf("进程按顺序运行依次为:\n");
    for (int i = 0; i < n; i++)
        printf("%s%s", pcbs[order[i]].name, (i == n - 1) ? "\n" : " ");
}

//-------------------------------------------------------------
// 算法评价：计算平均等待时间和平均周转时间
//   等待时间 = 周转时间 - 共需占用 CPU 时间
void statistics(int order[], int n)
{
    double sumWait = 0.0, sumTurn = 0.0;
    int finish[N];

    // order[] 中记录的是进程的执行次序，据此确定完成时间
    for (int k = 0; k < n; k++)
    {
        int idx = order[k];
        int done = 0;
        for (int j = 0; j <= k; j++)     // 该进程结束前已运行的总 CPU 时间
            done += pcbs[order[j]].time;
        finish[idx] = done;              // 该进程的完成时刻（同时刻到达时按次序累加）
        sumTurn += finish[idx];          // 周转时间 = 完成时间 - 到达时间（到达时间均为 0）
        sumWait += finish[idx] - pcbs[idx].time;
    }

    showOrder(order, n);
    printf("平均等待时间是:\n");
    printf("%f\n", sumWait / n);
    printf("平均周转时间是:\n");
    printf("%f\n", sumTurn / n);
}

//-------------------------------------------------------------
// 1) 先来先服务算法 FCFS：按进程进入的先后次序依次执行
void fncfs()
{
    printf("\n");
    reset();

    for (int i = 0; i < profNum; i++)
    {
        prof[i].need = 0;              // 一个接一个地执行完
        prof[i].status = FINISH;       // 该进程执行完毕
        printf("\n进程 %s 已经执行完毕 ！\n", prof[i].name);
        showTable(1);
    }

    int order[N];
    for (int i = 0; i < profNum; i++)
        order[i] = i;
    statistics(order, profNum);

    printf("所有进程都已经执行完毕 ！\n");
}

//-------------------------------------------------------------
// 2) 时间片轮转算法 RR
void rrobin()
{
    printf("\n");
    reset();

    int queue[N * 40];   // 就绪队列
    int front = 0, rear = 0;
    for (int i = 0; i < profNum; i++)   // 所有进程按次序进入就绪队列
        queue[rear++] = i;

    int order[N * 40];   // 运行次序
    int orderNum = 0;

    while (front < rear)
    {
        int cur = queue[front++];       // 取队首进程
        if (prof[cur].status == FINISH)
            continue;

        int run = (prof[cur].need < timeSlice) ? prof[cur].need : timeSlice;
        prof[cur].need -= run;          // 占用一个时间片
        order[orderNum++] = cur;

        printf("进程 %s 正在运行\n", prof[cur].name);
        if (prof[cur].need == 0)
        {
            prof[cur].status = FINISH;
            printf("进程 %s 已经执行完毕 ！\n", prof[cur].name);
        }
        else if (prof[cur].pri > 0)     // 运行后优先级提高
        {
            prof[cur].pri--;
        }
        showTable(2);

        if (prof[cur].status != FINISH)
            queue[rear++] = cur;        // 未完成，重新排到队尾
    }

    statistics(order, orderNum);

    printf("所有进程都已经执行完毕 ！\n");
}

//-------------------------------------------------------------
// 3) 优先级调度算法：每次选择优先级最高（值最小）的进程
void priorSched()
{
    printf("\n");
    reset();

    int order[N];
    for (int k = 0; k < profNum; k++)
    {
        int sel = -1;
        for (int i = 0; i < profNum; i++)
        {
            if (prof[i].status == FINISH)
                continue;
            if (sel == -1 || prof[i].pri < prof[sel].pri)
                sel = i;
        }

        printf("进程 %s 正在运行\n", prof[sel].name);
        prof[sel].need = 0;
        prof[sel].status = FINISH;
        order[k] = sel;

        printf("进程 %s 已经执行完毕 ！\n", prof[sel].name);
        showTable(3);
    }

    statistics(order, profNum);

    printf("所有进程都已经执行完毕 ！\n");
}

//-------------------------------------------------------------
// 4) 最短作业优先算法 SJF：每次选择共需占用 CPU 时间最短的进程
void sjf()
{
    printf("\n");
    reset();

    int order[N];
    for (int k = 0; k < profNum; k++)
    {
        int sel = -1;
        for (int i = 0; i < profNum; i++)
        {
            if (prof[i].status == FINISH)
                continue;
            if (sel == -1 || prof[i].time < prof[sel].time)
                sel = i;
        }

        printf("进程 %s 正在运行\n", prof[sel].name);
        prof[sel].need = 0;
        prof[sel].status = FINISH;
        order[k] = sel;

        printf("进程 %s 已经执行完毕 ！\n", prof[sel].name);
        showTable(4);
    }

    statistics(order, profNum);

    printf("所有进程都已经执行完毕 ！\n");
}

//-------------------------------------------------------------
// 5) 最短剩余时间优先算法 SRTF（抢占式）
//    本算法用指针来描述进程：p 始终指向“剩余时间最短”的那个进程。
void srtf()
{
    printf("\n");
    reset();

    PCB *p = NULL;                 // p 指向当前被选中的进程
    int order[N];
    int orderNum = 0;
    int finished = 0;

    while (finished < profNum)
    {
        // 在当前已到达且未完成的进程中，选出剩余时间最短者
        p = NULL;
        for (PCB *q = prof; q < prof + profNum; q++)
        {
            if (q->need <= 0 || q->arrive > nowTime)
                continue;
            if (p == NULL || q->need < p->need)
                p = q;
        }

        if (p == NULL)             // 暂无进程到达，时间向前推进
        {
            nowTime++;
            continue;
        }

        printf("进程 %s 正在运行\n", p->name);
        p->need--;                 // 运行一个时间单位
        order[orderNum++] = (int)(p - prof);
        nowTime++;

        if (p->need == 0)          // 该进程执行完毕
        {
            p->status = FINISH;
            finished++;
            printf("进程 %s 已经执行完毕 ！\n", p->name);
        }
        showTable(5);
    }

    statistics(order, orderNum);

    printf("所有进程都已经执行完毕 ！\n");
}

//-------------------------------------------------------------
// 主函数
int main()
{
    int alg = selectAlgorithm();   // 选择调度算法

    inputProcess();                // 输入进程信息

    switch (alg)
    {
    case 1:
        fncfs();                   // 先来先服务
        break;
    case 2:
        rrobin();                  // 时间片轮转
        break;
    case 3:
        priorSched();              // 优先级调度
        break;
    case 4:
        sjf();                     // 最短作业优先
        break;
    case 5:
        srtf();                    // 最短剩余时间优先
        break;
    default:
        printf("输入有误，请重新运行程序！\n");
        break;
    }

    printf("请按任意键继续. . .");
    getchar();
    getchar();
    return 0;
}
