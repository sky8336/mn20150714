/*
 * C语言打造——迷宫游戏
 * 迷宫游戏，先做出来迷宫界面，然后，记住从起点到出口的路线，从黑暗中走出去，
 * 用灯光，直接去摸索，走出去，每一次运行的界面都是随机的，代码比较多，暂时发一部分.
 *
 * 运用到的知识点有： 迷宫随机生成的算法 、数组、图形编程的显存渲染。
 *
 * 写项目代码循环敲，并不是那么重要，重要的是思路！
 *
 */

#include <easyx.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

// 定义常量

#define PI3.141592653589// 圆周率

#define UNIT_GROUND0// 表示地面
#define UNIT_WALL1// 表示墙

#define LIGHT_API / 3// 灯光的角度范围
#define LIGHT_R120// 灯光的照射距离

#define WIDTH480// 矿井的宽度
#define HEIGHT480// 矿井的高度

#define SCREENWIDTH640// 屏幕宽度
#define SCREENHEIGHT480// 屏幕高度

#define UNIT20// 每个墙壁单位的大小

#define PLAYER_R5// 游戏者的半径

// 定义常量

const SIZEg_utMap = { 23, 23 };// 矿井地图的尺寸(基于 UNIT 单位)
const POINTg_utPlayer = { 1, 1 };// 游戏者的位置(基于 UNIT 单位)
const POINTg_utExit = { 21, 22 };// 出口位置(基于 UNIT 单位)
const POINTg_ptOffset = { 10, 10 };// 矿井显示在屏幕上的偏移量

//////////////////////////////////////////////////////

// 定义全局变量

//

POINT g_ptPlayer;// 游戏者的位置
POINT g_ptMouse;// 鼠标位置

IMAGE g_imgMap(WIDTH, HEIGHT);// 矿井平面图
DWORD* g_bufMap;// 矿井平面图的显存指针

IMAGE g_imgRender(WIDTH, HEIGHT);// 渲染平面图
DWORD* g_bufRender;// 渲染平面图的显存指针
DWORD* g_bufScreen;// 屏幕的显存指针

// 枚举用户的控制命令

enum CMD {
	CMD_QUIT = 1,
	CMD_UP = 2,
	CMD_DOWN = 4,
	CMD_LEFT = 8,
	CMD_RIGHT = 16,
	CMD_RESTART = 32 
};

//////////////////////////////////////////////////////

// 函数声明

//

// 初始化

void Welcome();// 绘制游戏界面

void ReadyGo();// 准备开始游戏

void InitGame();// 初始化游戏数据

// 矿井生成

void MakeMaze(int width, int height);// 初始化(注：宽高必须是奇数)

void TravelMaze(int x, int y, BYTE** aryMap);// 遍历 (x, y) 四周

void DrawWall(int x, int y, bool left, bool top, bool right, bool bottom);

// 画一面墙

// 绘制

void Paint();// 绘制视野范围内的矿井

void Lighting(int _x, int _y, double _a);// 在指定位置和角度“照明”

void DrawPlayer();// 绘制游戏者

void DrawExit();// 绘制出口

// 处理用户控制

int GetCmd();// 获取用户输入的命令

void OnUp();// 向上移动

void OnLeft();// 向左移动

void OnRight();// 向右移动

void OnDown();// 向下移动

bool CheckWin();// 检查是否到出口

//////////////////////////////////////////////////////

// 函数定义

//

// 主程序

void main()
{

	// 初始化

	initgraph(SCREENWIDTH, SCREENHEIGHT);// 创建绘图窗口

	srand((unsigned)time(NULL));// 设置随机种子

	// 显示主界面

	Welcome();

	// 游戏过程

	int c;

	do {

		ReadyGo();

		while (true) {

			// 获得用户输入

			c = GetCmd();

			// 处理用户输入

			if (c & CMD_UP)
				OnUp();

			if (c & CMD_DOWN)
				OnDown();

			if (c & CMD_LEFT)
				 OnLeft();

			if (c & CMD_RIGHT)
				OnRight();

			if (c & CMD_RESTART) {
				if (MessageBox(GetHWnd(), _T("您要重来一局吗？"), _T("询问"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
					break;
			}

			if (c & CMD_QUIT) {
				if (MessageBox(GetHWnd(), _T("您确定要退出游戏吗？"), _T("询问"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
					break;
			}

			// 绘制场景

			Paint();

			// 判断是否走出矿井

			if (CheckWin()) {

				// 是否再来一局

				HWND hwnd = GetHWnd();

				if (MessageBox(hwnd, _T("恭喜你走出来了！\n您想再来一局吗？"), _T("恭喜"), MB_YESNO | MB_ICONQUESTION) != IDYES)
					c = CMD_QUIT;

				break;
			}

			// 延时

			Sleep(16);
		}

	} while (!(c & CMD_QUIT));

	// 关闭图形模式

	closegraph();
}

// 准备开始游戏

void ReadyGo()
{

	// 初始化

	InitGame();

	// 停电前兆

	int time[7] = { 1000, 50, 500, 50, 50, 50, 50 };//设置7次闪光

	int i, x, y;

	for (i = 0; i < 7; i++) {
		if (i % 2 == 0) {
			putimage(0, 0, &g_imgMap);

			DrawPlayer();

			DrawExit();
		} else
			clearrectangle(0, 0, WIDTH, HEIGHT);

		Sleep(time[i]);
	}

	// 电力缓慢中断
	for (i = 255; i >= 0; i -= 5) {
		for (y = (HEIGHT - 1) * SCREENWIDTH; y >= 0; y -= SCREENWIDTH)
			for (x = 0; x < WIDTH; x++)
				if (g_bufScreen[y + x] != 0)
					g_bufScreen[y + x] -=0x050505;

		FlushBatchDraw();

		DrawPlayer();

		DrawExit();

		Sleep(50);
	}

	// 绘制游戏区
	Paint();
}
