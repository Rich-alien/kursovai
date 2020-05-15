#include "pch.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <GL\freeglut.h>
#include <math.h>
#include <algorithm>
#include <time.h>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>

using namespace std;

const double OFFSET = 0.01;

double MAX_Y; //максимальное значение функции
double MIN_Y; //минимальное значение функции
double MAX_X; //не используется
double integral; //тут хранится вычисленное значение интеграла
double leftBorder = 0;	//левая граница отрезка интегрирования (задается пользователем)
double rightBorder = 6.28;	//левая граница отрезка интегрирования (задается пользователем)
long double TIME = 0;	//время работы алгоритма

int menuItem;	//для менюшки
int X_LEFT_COORD;	//для отрисовки (то же самое, что leftBorder)
int X_RIGHT_COORD;	//для отрисовки (то же самое, что rightBorder)
int Y_COORD;
int THREAD_COUNT = 2;	//количество потоков (задает пользователь), по дефолту 2

atomic<int> generalPointsCounter = 0;	//считаем общее количество генерируемых точек
atomic<int> pointsInAreaCounter = 0;	//считаем точки, попавшие под график

char fun1[] = "(x^0.25)*exp(-x)";	//в таком виде храним название функции, чтобы выводить его в окне OpenGL
char fun2[] = "cos(x)*cos(x^2)+1";
char fun3[] = "sqrt(abs(sin(x)))";

//указатель на функцию, интеграл от которой вычисляется
//такая реализация избавляет от кучи ненужных if в участке кода, где происходит интегрирование
double(*functionToIntegration)(double x) = nullptr;

//печать текста в окне OpenGL
void glutPrint(float x, float y, void* font, char* text, float red, float green, float blue)
{
	glColor3f(red, green, blue);
	glRasterPos2f(x, y);
	for (char *c = text; *c != '\0'; c++)
	{
		glutBitmapCharacter(font, *c);
	}
}

//считаем интеграл этих функций
double func1(double x)
{
	return sqrt(sqrt(x))*exp(-x);
}

double func2(double x)
{
	return cos(x)*cos(x*x)+1;
}

double func3(double x)
{
	return sqrt(abs(sin(x)));
}

//поиск максимального и минимального значения функции
void SetMaxY()
{
	MAX_Y = 0;
	MIN_Y = 0;
	double x = leftBorder;
	while (x < rightBorder)
	{
		double f = functionToIntegration(x);
		if (f > MAX_Y)
			MAX_Y = f;
		if (f < MIN_Y)
			MIN_Y = f;
		x += OFFSET;
	}
	if (MIN_Y > 0) MIN_Y = 0;
}

//обработка изменения размера окна OpenGL
void reshape(int width, int height)
{
	const float ar = (float)width / (float)height;

	glViewport(-10, -10, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(X_LEFT_COORD * ar, (X_LEFT_COORD + rightBorder + 10) * ar, MIN_Y * ar, Y_COORD * ar, 1.0, -1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

//рисуем ось Х
void drawOrtho()
{
	glBegin(GL_LINES);
	glColor3b(0, 0, 0);
	glVertex2d(X_LEFT_COORD, 0.0);
	glVertex2d(X_RIGHT_COORD, 0.0);
	glEnd();
	//засечка в точке (0;0)
	glBegin(GL_LINES);
	glColor3b(0, 0, 0);
	glVertex2d(0, -0.3);
	glVertex2d(0, 0.3);
	glEnd();
}

//рисуем график функции по точкам
void drawFunc()
{
	double x = leftBorder, y;
	glPointSize(3.0);
	glBegin(GL_POINTS);
	glColor3f(0.0, 0.0, 1.0);
	for (int i = 0; i < (rightBorder - leftBorder) * 100; i++)
	{
		y = functionToIntegration(x);
		glVertex2d(x, y);
		x += OFFSET;
	}
	glEnd();
	glFlush();
}

//рисуем прямоугольник, которым ограничивается график функции (радужная линия)
void drawRectangle()
{
	//если минимальное значение функции > 0, то нижняя грань прямоугольника будет лежать на оси Х, а не на значении MIN_Y
	if (MIN_Y >= 0)
	{
		glBegin(GL_LINES);
		glColor3f(1.0, 0.0, 0.0);
		glVertex2d(leftBorder, MAX_Y);
		glColor3f(1.0, 1.0, 0.0);
		glVertex2d(rightBorder, MAX_Y);

		glVertex2d(rightBorder, MAX_Y);
		glColor3f(0.0, 1.0, 0.0);
		glVertex2d(rightBorder, 0.0);

		glColor3f(0.0, 1.0, 1.0);
		glVertex2d(rightBorder, 0.0);
		glColor3f(0.0, 0.0, 1.0);
		glVertex2d(leftBorder, 0.0);


		glColor3f(1.0, 0.0, 1.0);
		glVertex2d(leftBorder, 0.0);
		glColor3f(1.0, 0.0, 0.0);
		glVertex2d(leftBorder, MAX_Y);
		glEnd();
	}
	else //в ином случае рисуем прямоугольник в соответствие с макс. и мин. значениями функции
	{
		glBegin(GL_LINES);
		glColor3f(1.0, 0.0, 0.0);
		glVertex2d(leftBorder, MAX_Y);
		glColor3f(1.0, 1.0, 0.0);
		glVertex2d(rightBorder, MAX_Y);

		glVertex2d(rightBorder, MAX_Y);
		glColor3f(0.0, 1.0, 0.0);
		glVertex2d(rightBorder, MIN_Y);

		glColor3f(0.0, 1.0, 1.0);
		glVertex2d(rightBorder, MIN_Y);
		glColor3f(0.0, 0.0, 1.0);
		glVertex2d(leftBorder, MIN_Y);


		glColor3f(1.0, 0.0, 1.0);
		glVertex2d(leftBorder, MIN_Y);
		glColor3f(1.0, 0.0, 0.0);
		glVertex2d(leftBorder, MAX_Y);
		glEnd();
	}
	glFlush();
}

double *xcoord, *ycoord; //здесь будут созданы массивы, в которых хранятся координаты Х и У
bool *color;	//тут будет массив с цветами точек

//тут генерируем точки и проверяем, куда они попали - под график или над ним.
void f(double start_x, double end_x, int points, int startIndex)
{
	int pcounter = 0;
	for (int i = startIndex; i < startIndex + points; i++)
	{
		//рандомим координаты
		//Х генерим так, чтобы он попал в отрезок, заданный пользователем
		xcoord[i] = start_x + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (end_x - start_x)));
		//У генерим так, чтобы он попал в диапазон между макс. и мин. значением функции
		ycoord[i] = MIN_Y + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (MAX_Y - MIN_Y)));

		double res = functionToIntegration(xcoord[i]);	//вычисляем значение функции в точке Х, которую перед этим сгенерили
		color[i] = 0;
		//проверяем, куда попала точка. если выполниться один из этих ифов, то это значит, что точка попала под график и ее цвет будет оранжевым
		if (res > 0)
		{
			if ((ycoord[i] <= res) && (ycoord[i] > 0))
			{
				//увеличиваем счетчик точек, попавших под график
				pcounter++;
				color[i] = 1;
			}
		}
		else if (res <= 0)
		{
			if ((ycoord[i] >= res) && (ycoord[i] <= 0))
			{
				pcounter++;
				color[i] = 1;
			}
		}
	}
	pointsInAreaCounter = pointsInAreaCounter + pcounter;
}

void g(double start_x, double end_x, int points, int startIndex)
{
	double xc, yc, c;
	int piac = 0, gpc = 0;
	for (int i = startIndex; i < startIndex + points; i++)
	{
		xc = start_x + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (end_x - start_x)));
		yc = MIN_Y + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (MAX_Y - MIN_Y)));

		double res = functionToIntegration(xc);
		c = 0;
		if (res > 0)
		{
			if ((yc <= res) && (yc > 0))
			{
				//pointsInAreaCounter++;
				piac++;
				c = 1;
			}
		}
		else if (res <= 0)
		{
			if ((yc >= res) && (yc <= 0))
			{
				//pointsInAreaCounter++;
				piac++;
				c = 1;
			}
		}
		//generalPointsCounter++;
		gpc++;
	}
	pointsInAreaCounter = pointsInAreaCounter + piac;
	generalPointsCounter = generalPointsCounter + gpc;
}

//рисуем точки. в этой функции происходит большая часть метода монте-карло
void drawPoints()
{
	srand(time(0));
	double rectangleArea = (MAX_Y - MIN_Y)*(rightBorder - leftBorder);	//вычисляем площадь прямоугольника, который ограничивает функцию на графике
	int points = 4000 * rectangleArea;	//задаем количество точек
	generalPointsCounter = points;
	int pointsIn1Thread = points / THREAD_COUNT;	//делим работу по генерации точек между потоками

	xcoord = new double[points];
	ycoord = new double[points];
	color = new bool[points];

	vector<thread> threads;	//в этот контейнер будем создавать потоки
	threads.clear();

	pointsInAreaCounter = 0;

	double step = (rightBorder - leftBorder) / THREAD_COUNT;	//вычисляем длину участка, на котором будет работать поток
	chrono::duration<double, std::milli> atime;	//сюда записываем время работы
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		double strt = leftBorder + step * (double)i;	//вычисляем начало и конец участка, на котором будет работать поток
		double end = strt + step;
		int index = pointsIn1Thread * i;	//вычисляем индекс в массиве координат и цветов, начиная с которого поток будет писать данные
		threads.push_back(thread(f, strt, end, pointsIn1Thread, index));	//создаем поток
	}
	for (int i = 0; i < threads.size(); i++)
	{
		threads[i].join();	//завершаем работу потоков
	}
	atime = std::chrono::high_resolution_clock::now() - start;
	TIME = atime.count();	//вытаскиваем время работы в привычный double

	threads.clear();
	//рисуем точка
	glPointSize(1.0);
	glBegin(GL_POINTS);
	glColor3f(1.0, 0.0, 0.0);
	for (int i = 0; i < points; i++)
	{
		if (color[i] == 0) glColor3f(1.0f, 0.0f, 0.0f);
		else glColor3f(1.0f, 0.7f, 0.0f);

		glVertex2f(xcoord[i], ycoord[i]);
	}
	glEnd();
	delete[] xcoord;	//освобождаем память
	delete[] ycoord;
	delete[] color;
	glFlush();
}

void Test(int iterationsCount = 1, int pointsCount = 1000, int threadNum = 2)
{
	ofstream logs("D:\\tests_cpp.txt", ios_base::app);

	functionToIntegration = func3;
	int pointsIn1Thread = pointsCount / threadNum;

	SetMaxY();

	cout << "\n** Computing... **\n";

	TIME = clock();
	vector<thread> threads;
	threads.clear();

	double step = (rightBorder - leftBorder) / threadNum;
	for (int j = 0; j < iterationsCount; j++)
	{
		pointsInAreaCounter = 0;
		generalPointsCounter = 0;
		for (int i = 0; i < threadNum; i++)
		{
			double strt = leftBorder + step * (double)i;
			double end = strt + step;
			int index = pointsIn1Thread * i;
			threads.push_back(thread(g, strt, end, pointsIn1Thread, index));
		}
		for (int i = 0; i < threads.size(); i++)
		{
			threads[i].join();
		}
		threads.clear();
	}
	TIME = (clock() - TIME) / iterationsCount;
	TIME /= 1000;	//перевод в секунды

	logs << "\n\nAverage time: " << TIME;
	logs << "\nIterations: " << iterationsCount;
	logs << "\nPoints: " << pointsCount;
	logs << "\nThreads: " << threadNum;
	logs.close();
}

//обработка событий, связаных с мышью
void Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			if ((x >= 580) && (y <= 20))	//по нажатию в правый верхний угол окно OpenGL закроется
			{
				glutLeaveMainLoop();
			}
		}
	}
}

//рисуем всё, что было описано выше
void Display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	drawRectangle();
	drawFunc();
	drawOrtho();
	drawPoints();
	drawFunc();

	double rectangleArea = (MAX_Y - MIN_Y)*(rightBorder - leftBorder);
	//финальная часть монте-карло - вычисление значения интеграла
	integral = rectangleArea * static_cast<double>(pointsInAreaCounter) / static_cast<double>(generalPointsCounter);
	cout << "Интеграл = " << integral << endl;
	cout << "Количество потоков: " << THREAD_COUNT << ". Время: " << TIME << " мс\n";

	//отрисовка границ диапазона интегрирования (числа) и названия функции
	char lb[10], rb[10], nol[] = "0", qqq[] = "???";
	_itoa_s(rightBorder, rb, 10, 10);
	_itoa_s(leftBorder, lb, 10, 10);
	glutPrint(0, 0, GLUT_BITMAP_HELVETICA_18, nol, 0.0f, 0.0f, 0.0f);   //0
	glutPrint(leftBorder, 0, GLUT_BITMAP_HELVETICA_18, lb, 0.0f, 0.0f, 0.0f);   //левая граница
	glutPrint(rightBorder, 0, GLUT_BITMAP_HELVETICA_18, rb, 0.0f, 0.0f, 0.0f);  //правая граница
	//название
	if (functionToIntegration == func1) glutPrint((leftBorder + rightBorder) / 2, MAX_Y, GLUT_BITMAP_HELVETICA_18, fun1, 0.0f, 0.0f, 0.0f);
	else if (functionToIntegration == func2) glutPrint((leftBorder + rightBorder) / 2, MAX_Y, GLUT_BITMAP_HELVETICA_18, fun2, 0.0f, 0.0f, 0.0f);
	else if (functionToIntegration == func3) glutPrint((leftBorder + rightBorder) / 2, MAX_Y, GLUT_BITMAP_HELVETICA_18, fun3, 0.0f, 0.0f, 0.0f);
	else glutPrint(0, MAX_Y, GLUT_BITMAP_HELVETICA_18, qqq, 0.0f, 0.0f, 0.0f);

	glFlush();
}

void PrintMenu()
{
	cout << "1. О методе Монте Карло\n";
	cout << "2. Выбрать функцию для интегрирования\n";
	cout << "3. Перейти к визуализации\n";
	cout << "4. Задать количество потоков (текущее: " << THREAD_COUNT << ")\n";
	cout << "5. Тест\n";
	cout << "6. Выход.\n";
}

//вывод первого пункта меню
void PrintInfo()
{
	ifstream INFO;
	INFO.open("info.txt", ios_base::in);
	if (INFO.is_open() == true)
	{
		string buffer;
		while (!INFO.eof())
		{
			getline(INFO, buffer);
			cout << buffer << endl;
			buffer = "";
		}
	}
	else
	{
		cout << "Не удалось открыть файл.\n";
	}
	INFO.close();
}

//выбор функции (2-й пункт меню)
void ChooseFunction()
{
	int choise = 0;
	while (choise < 1 || choise > 3)
	{
		cout << "Доступные функции:\n";
		cout << "1. sqrt(sqrt(x))*exp(-x)\n";
		cout << "2. cos(x)*cos(x*x)+1\n";
		cout << "3. sqrt(abs(sin(x)))\n";
		cout << "Введите число от 1 до 3:\n";
		cin >> choise;
	}
	switch (choise)
	{
	case 1: {functionToIntegration = func1; break; }
	case 2: {functionToIntegration = func2; break; }
	case 3: {functionToIntegration = func3; break; }
	default: break;
	}
	cout << "\nВведите диапазон интегрирования (без квадратных скобок) [a b]:\n";
	cin >> leftBorder >> rightBorder;
	SetMaxY();
	X_LEFT_COORD = leftBorder - 5;
	X_RIGHT_COORD = rightBorder + 5;
	Y_COORD = MAX_Y + 5;
}

void CreateTest()
{
	/*int params[3];
	cout << "Введите количество итераций, точек и потоков через пробел:\n";
	cin >> params[0] >> params[1] >> params[2];
	Test(params[0], params[1], params[2]);*/
	for (int j = 2; j <= 4; j++)
	{
		int params[] = { 1000, 10000, j };
		for (int i = 0; i < 50; i++, params[1] += 10000)
		{
			Test(params[0], params[1], params[2]);
		}
	}
}

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "Russian");
	bool start = false;
	while (!start)
	{
		cout << "***Вычисление интеграла методом Монте Карло***\n";
		PrintMenu();
		cin >> menuItem;
		switch (menuItem)
		{
		case 1: {PrintInfo(); break; }
		case 2: {ChooseFunction(); break; }
		case 3:
		{
			if (functionToIntegration == nullptr)
			{
				cout << "Сначала нужно выбрать функцию\n";
				break;
			}
			cout << "\nСоздание окна...\n";

			glutInit(&argc, argv);
			glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
			glutInitWindowSize(1400, 600);
			glutInitWindowPosition(0, 0);
			glutCreateWindow("MONTE_CARLO_VISUALISATION");

			glClearColor(1.0, 1.0, 1.0, 1.0);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			glOrtho(X_LEFT_COORD, X_RIGHT_COORD, MIN_Y, Y_COORD, 1.0, -1.0);

			//glutReshapeFunc(reshape);
			glutDisplayFunc(Display);

			glutMouseFunc(Mouse);
			glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

			glutMainLoop();

			break;
		}
		case 4:
		{
			cout << "Введите количество потоков: \n";
			cin >> THREAD_COUNT;
			//if (THREAD_COUNT > omp_get_max_threads()) THREAD_COUNT = omp_get_max_threads();
			if (THREAD_COUNT <= 0) THREAD_COUNT = 1;
			break;
		}
		case 5: {CreateTest(); break; }
		case 6: {start = true; break; }
		default: {cout << "Некорректный ввод. Попробуйте еще раз.\n"; }
		}
	}

	return 0;
}

