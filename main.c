/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
						针对源码进行了改进
						通过原有的TestABC
						写上可以进行操作的shell
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Xuejinwei, 2018
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task* p_task;
	struct proc* p_proc= proc_table;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16   selector_ldt = SELECTOR_LDT_FIRST;
        u8    privilege;
        u8    rpl;
	int   eflags;
	int   i, j;
	int   prio;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
	        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio      = 15;
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
			prio      = 5;
                }

		strcpy(p_proc->name, p_task->name);	/* name of the process */
		p_proc->pid = i;			/* pid */

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

        

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int fd;
	int i, n;

	char tty_name[] = "/dev_tty0";

	char rdbuf[128];


	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);


	const char bufw[80] = {0};

	
	clear();
	printf("                        ==================================\n");
	printf("                                   Xinux v1.0.0             \n");
	printf("                                 Kernel on Orange's \n");
	printf("                                     Welcome !\n");
	printf("                        ==================================\n");
	
	while (1) {
		printl("[root@localhost /] ");
		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		if (!strcmp(rdbuf, "cal"))
		{
			int year;
			char temp[70];
			printf("input the year:");
			int r = read(fd_stdin, temp, 70);
			temp[r] = 0;
			atoi(temp, &year);
			Calendar(year);
			printf("\n");
			continue;
		}
        else if (!strcmp(rdbuf, "proc"))
        {
			ProcessManage();
        }
		else if (!strcmp(rdbuf, "flm"))
		{
			printf("File Manager is already running on TTY1 ! \n");
			continue;

		}else if (!strcmp(rdbuf, "help"))
		{
			help();
		}
		else if (!strcmp(rdbuf, "game1"))
		{

			Game1(fd_stdin, fd_stdout);
		}
		
		else if (strcmp(rdbuf, "cl") == 0)
		{
			clear();
			printf("                        ==================================\n");
			printf("                                   Xinux v1.0.0            \n");
			printf("                                 Kernel on Orange's \n\n");
			printf("                                     Welcome !\n");
			printf("                        ==================================\n");
		}
		

		else
			printf("Command not found, please input HL to get help!\n");
	}

}


void help()
{
	printf("=============================================================================\n");
	printf("Command List     :\n");
	printf("1. proc       : A process manage,show you all process-info here\n");
	printf("2. flm        : Run the file manager\n");
	printf("3. cl         : Clear the screen\n");
	printf("4. help       : Show operation guide\n");
	printf("5. cal        : Show a calendar\n");
	printf("6. game1      : Run a small game(guess number) on this OS\n");
	printf("==============================================================================\n");
}

/*======================================================================*
Calendar
日历生成相关函数
*======================================================================*/

/*思路:（1）首先需要打印年月和月历的周一到周日（2）判断每个月的1号是周几，这样利用固定的算法就可以依次求出2、3、4、、、等是星期几（3）其中还需要判断在什么时候进行换行处理。以及判断 是否是闰年。*/

int f(int year, int month)
{/*f(年，月)＝年－1，如月<3;否则，f(年，月)＝年*/
	if (month<3) return year - 1;
	else return year;
}

int g(int month)
{/*g(月)＝月＋13，如月<3;否则，g(月)＝月＋1*/
	if (month<3) return month + 13;
	else return month + 1;
}


/*计算日期的N值*/
int n(int year, int month, int day)
{
	/*N=1461*f(年、月)/4+153*g(月)/5+日*/
	return 1461L * f(year, month) / 4 + 153L * g(month) / 5 + day;
}

/*利用N值算出某年某月某日对应的星期几*/
int w(int year, int month, int day)
{
	/*w=(N-621049)%7(0<=w<7)*/
	return(int)((n(year, month, day) % 7 - 621049L % 7 + 7) % 7);
}

int date[12][6][7];

/*该数组对应了非闰月和闰月的每个月份的天数情况*/
int day_month[][12] = { { 31,28,31,30,31,30,31,31,30,31,30,31 },
{ 31,29,31,30,31,30,31,31,30,31,30,31 } };

void Calendar(int year)
{
	int sw, leap, i, j, k, wd, day;/*leap 判断闰年*/

	char title[] = "SUN MON TUE WED THU FRI SAT";


	sw = w(year, 1, 1);
	leap = year % 4 == 0 && year % 100 || year % 400 == 0;/*判闰年*/
	for (i = 0; i<12; i++)
		for (j = 0; j<6; j++)
			for (k = 0; k<7; k++)
				date[i][j][k] = 0;/*日期表置0*/
	for (i = 0; i<12; i++)/*一年十二个月*/
	{
		for (wd = 0, day = 1; day <= day_month[leap][i]; day++)
		{/*将第i＋1月的日期填入日期表*/
			date[i][wd][sw] = day;
			sw = ++sw % 7;/*每星期七天，以0至6计数*/
			if (sw == 0) wd++;/*日期表每七天一行，星期天开始新的一行*/
		}
	}

	printf("\n|==================The Calendar of Year %d =====================|\n|", year);

	for (i = 0; i<6; i++)
	{/*先测算第i+1月和第i+7月的最大星期数*/
		for (wd = 0, k = 0; k<7; k++)/*日期表的第六行有日期，则wd!=0*/
			wd += date[i][5][k] + date[i + 6][5][k];
		wd = wd ? 6 : 5;
		printf("%2d  %s  %2d  %s |\n|", i + 1, title, i + 7, title);
		for (j = 0; j<wd; j++)
		{
			printf("   ");/*输出四个空白符*/
						  /*左栏为第i+1月，右栏为第i+7月*/
			for (k = 0; k<7; k++)
				if (date[i][j][k])
					printf("%4d", date[i][j][k]);
				else printf("    ");
				printf("     ");/*输出十个空白符*/
				for (k = 0; k<7; k++)
					if (date[i + 6][j][k])
						printf("%4d", date[i + 6][j][k]);
					else printf("    ");
					printf(" |\n|");
		}


	}
	printf("=================================================================|\n");

}

/*======================================================================*
小游戏1 猜数字
*======================================================================*/
void Game1(int fd_stdin, int fd_stdout) {
	int result = 2018;
	int finish = 0;
	int guess;
	printf("Now the guess number game begin\n");
	while (!finish) {
		printf("please input your guess number:");
		char temp[70];
		int r = read(fd_stdin, temp, 70);
		temp[r] = 0;
		atoi(temp, &guess);
		if (guess < result) {
			printf("your number is small\n");
		}
		else if (guess>result) {
			printf("your number is big\n");
		}
		else {
			printf("Congratulations,you're right\n");
			finish = 1;
		}

	}
}

/*======================================================================*
                               TestB
							文件系统管理
 *======================================================================*/

/*
文件系统较为简洁
在调用oranges系统已有的文件操作基础上进行了界面设计(参阅/fs/文件夹)
仅提供文件创建,文件删除，文件打开，文件读写等操作
*/

void TestB()
{
	char tty_name[] = "/dev_tty1";

	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	/*打印界面*/
	printf("                        ==================================\n");
	printf("                                    File Manager           \n");
	printf("                                 Kernel on Orange's \n\n");
	printf("                        ==================================\n");

	/*定义相关参数*/

	char file_name[128];//操作的文件名
	char cmd[8];//进行的操作
	char rdbuf[128];


	char created_table[100][20] = { 0 };//已存在的文件列表 最大存在100个文件
	int numOfcreate = 0;


	while (1) {
		printf("$ ");

		/*读取用户输入*/
		int r = read(fd_stdin, rdbuf, 80);
		rdbuf[r] = 0;

		/*进行判断*/
		if (!strcmp(rdbuf, "help")) {
			help_b();
			continue;
		}
		else if (!strcmp(rdbuf, "ls")) {
			if (!numOfcreate) {
				printf("no file\n");
				continue;
			}
			for (int i = 0; i < numOfcreate; i++) {
				printf("%s    ", created_table[i]);
				if (i!=0&&(i % 5) == 0) {
					printf("\n");
				}
			}
			printf("\n");
			continue;
		}
		else {
			/*将用户输入分解成cmd+filename*/
			int i = 0;
			int j = 0;
			while (rdbuf[i] != ' ') {
				cmd[i] = rdbuf[i];
				i++;
				if (i > 128)break;
			}
			if (i > 128) {
				printf("Command not found, please input help to get help!\n");
				continue;
			}

			cmd[i++] = 0;

			while (rdbuf[i] != 0) {
				file_name[j] = rdbuf[i];
				i++;
				j++;
			}
			file_name[j] = 0;

			/*开始执行命令*/
			if (!strcmp(cmd, "cre")) {
				if (create_file(file_name)) {
					/*文件列表没有该文件*/
					memcpy(created_table[numOfcreate], file_name, 20);
					numOfcreate++;
					continue;
				}
			}
			else if (!strcmp(cmd, "rd")) {
				read_file(file_name);
				continue;
			}
			else if (!strcmp(cmd, "wt")) {
				write_file(file_name, fd_stdin);
				continue;
			}
			else if (!strcmp(cmd, "del")) {
				del_file(file_name);
				continue;
			}
			else {
				printf("Command not found, please input help to get help!\n");
				continue;
			}
		}
	}
}


void help_b() {
	printf("=============================================================================\n");
	printf("Command List     :\n");
	printf("1. ls [filename]        : list the all files \n");
	printf("1. cre [filename]       : Create a new file \n");
	printf("2. rd [filename]        : Read the file\n");
	printf("3. wt [filename]        : Write at the end of the file\n");
	printf("4. del [filename]       : Delete the file\n");
	printf("5. help                 : Show operation guide\n");
	printf("==============================================================================\n");
}

/*======================================================================*
文件操作相关函数
*======================================================================*/

int create_file(char* file_name) {
	int fd;
	/*调用Orange系统写好的文件系统相关接口函数*/
	fd = open(file_name, O_CREAT | O_RDWR);
	if(fd==-1){
		printf("Create file failed! the file has been existed.\n");
		return -1;
	}
	
	char buf[128];
	buf[0] = 0;
	write(fd, buf, 1);
	printf("File create successful: %s (fd %d)\n", file_name, fd);
	close(fd);
	return 1;
}

int read_file(char* file_name) {
	int fd;
	/*调用Orange系统写好的文件系统相关接口函数*/
	fd = open(file_name, O_RDWR);
	if (fd ==-1) {
		printf("Open file failed! please check the filename \n");
		return -1;
	}

	char buf[1024];
	int n = read(fd, buf, 1024);
	printf("%s\n", buf);
	close(fd);
	return 1;
}

int write_file(char* file_name,int fd_stdin) {
	int fd;
	fd = open(file_name, O_RDWR);
	if (fd == -1)
	{
		printf("Open file failed! please check the filename \n");
		return -1;
	}
	char buf[128];
	int r = read(fd_stdin, buf, 80);
	buf[r] = 0;

	int m = write(fd, buf, r+ 1);
	close(fd);
	return 1;
}

int del_file(char* file_name) {
	int r = unlink(file_name);
	if (r == 0)
	{
		printf("File delete successful!\n");
		return 1;
	}
	else
	{
		printf("Delete file failed! Please check the fileaname!\n");
		return -1;
	}
}



void TestC()
{
	spin("TestC");
}




/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

void clear()
{
	clear_screen(0,console_table[current_console].cursor);
	console_table[current_console].crtc_start = 0;
	console_table[current_console].cursor = 0;
	
}




void ProcessManage()
{
}
