/* SEQUENTIAL */

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <omp.h>
#include <GL\freeglut.h>
#include <math.h>
#include <algorithm>
#include <time.h>


using namespace std;

const double OFFSET = 0.01;

double MAX_Y;
double MIN_Y;
double MAX_X;
double integral;
double leftBorder = 0;
double rightBorder = 6.28;
double TIME = 0;

int menuItem;
int X_LEFT_COORD;
int X_RIGHT_COORD;
int Y_COORD;
int generalPointsCounter = 0;
int pointsInAreaCounter = 0;

char fun1[] = "(x^0.25)*exp(-x)";
char fun2[] = "cos(x)*cos(x^2)+1";
char fun3[] = "sqrt(abs(sin(x)))";

double (*functionToIntegration)(double x) = nullptr;

void glutPrint(float x, float y, void* font, char* text, float red, float green, float blue)
{
    glColor3f(red,green,blue);
    glRasterPos2f(x,y);
    for (char *c = text; *c != '\0'; c++)
    {
        glutBitmapCharacter(font, *c);
    }
}

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

void reshape(int width, int height)
{
    const float ar = (float) width / (float) height;

    glViewport(-10, -10, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(X_LEFT_COORD * ar, (X_LEFT_COORD + rightBorder + 10) * ar, MIN_Y * ar, Y_COORD * ar, 1.0, -1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void drawOrtho()
{
    glBegin(GL_LINES);
        glColor3b(0,0,0);
        glVertex2d(X_LEFT_COORD, 0.0);
        glVertex2d(X_RIGHT_COORD, 0.0);
    glEnd();
    //засечка по центру
    glBegin(GL_LINES);
        glColor3b(0,0,0);
        glVertex2d(0, -0.3);
        glVertex2d(0, 0.3);
    glEnd();
}

void drawFunc()
{
    double x = leftBorder, y;
    glPointSize(3.0);
    glBegin(GL_POINTS);
        glColor3f(0.0, 0.0, 1.0);
        for (int i = 0; i < (rightBorder-leftBorder)*100; i++)
        {
            y = functionToIntegration(x);
            glVertex2d(x,y);
            x += OFFSET;
        }
    glEnd();
    glFlush();
}

void drawRectangle()
{
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
    else
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

double *xcoord, *ycoord;
bool *color;

void drawPoints()
{
    srand(time(0));
    double res;
    double rectangleArea = (MAX_Y - MIN_Y)*(rightBorder - leftBorder);
    int points = 4000 * rectangleArea;
    generalPointsCounter = points;

    xcoord = new double[points];    //массив, в котором хранятся координата х генерируемых точек
	ycoord = new double[points];    //массив, в котором хранятся координата у генерируемых точек
	color = new bool[points];       //массив, в котором хранятся цвета генерируемых точек

    pointsInAreaCounter = 0;

    TIME = omp_get_wtime();
    for (int i = 0; i < points; i++)
    {
        xcoord[i] = leftBorder + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX/(rightBorder - leftBorder)));
        ycoord[i] = MIN_Y + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX/(MAX_Y - MIN_Y)));

        color[i] = 0;   //красный цвет
        res = functionToIntegration(xcoord[i]);
        if (res > 0)    //определение, куда попала точка
        {
            if ((ycoord[i] <= res) && (ycoord[i] > 0))
                {
                    pointsInAreaCounter++;
                    color[i] = 1;   //желтый цвет
                }
        }
        else if (res <= 0)
        {
            if ((ycoord[i] >= res) && (ycoord[i] <= 0))
            {
                pointsInAreaCounter++;
                color[i] = 1;
            }
        }
    }
    TIME = omp_get_wtime() - TIME;

    glPointSize(1.0);
    glBegin(GL_POINTS);
    for (int i = 0; i < points; i++)
    {
        if (color[i] == 0) glColor3f(1.0f, 0.0f, 0.0f);
		else glColor3f(1.0f, 0.7f, 0.0f);

		glVertex2f(xcoord[i], ycoord[i]);
    }
    glEnd();
    delete[] xcoord;
	delete[] ycoord;
	delete[] color;
    glFlush();
}

void Test(int iterationsCount = 1, int pointsCount = 1000, int threadNum = 1)
{
    ofstream logs("tests.txt", ios_base::app);
    double  xc,
            yc,
            col,
            res;
    functionToIntegration = func3;
    SetMaxY();
    cout << "\n** Computing... **\n";
    TIME = omp_get_wtime();
    for (int it = 0; it < iterationsCount; it++)
    {
        pointsInAreaCounter = 0;
        generalPointsCounter = 0;
        for (int i = 0; i < pointsCount; i++)
        {
            xc = leftBorder + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX/(rightBorder - leftBorder)));
            yc = MIN_Y + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX/(MAX_Y - MIN_Y)));
            col = 0;
            res = functionToIntegration(xc);
            if (res > 0)
            {
                if ((yc <= res) && (yc > 0))
                    {
                        pointsInAreaCounter++;
                        col = 1;
                    }
            }
            else if (res <= 0)
            {
                if ((yc >= res) && (yc <= 0))
                {
                    pointsInAreaCounter++;
                    col = 1;
                }
            }
            generalPointsCounter++;
        }
    }
    TIME = (omp_get_wtime() - TIME) / (double)iterationsCount;
    cout << "** Done! **\n";
    logs << "\n\nAverage time: " << TIME;
    logs << "\nIterations: " << iterationsCount;
    logs << "\nPoints: " << pointsCount;
    logs << "\nThreads: " << threadNum;
    logs.close();
}

void Mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            if ((x >= 580) && (y <= 20))
            {
                glutLeaveMainLoop();
            }
        }
    }
}

void Display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    drawRectangle();
    drawFunc();
    drawOrtho();
    drawPoints();
    drawFunc();

    double rectangleArea = (MAX_Y - MIN_Y)*(rightBorder - leftBorder);
    integral = rectangleArea * static_cast<double>(pointsInAreaCounter) / static_cast<double>(generalPointsCounter);
    cout << "Интеграл = " << integral << endl;
    cout << "Время: " << TIME << " мс\n";

    //отрисовка границ диапазона интегрирования (числа) и названия функции
    char lb[5];
    char rb[5];
    char *pr = itoa(rightBorder, rb, 10);
    char *pl = itoa(leftBorder, lb, 10);
    glutPrint(0,0,GLUT_BITMAP_HELVETICA_18, "0",0.0,0.0,0.0);   //0
    glutPrint(leftBorder,0,GLUT_BITMAP_HELVETICA_18, pl,0.0,0.0,0.0);   //левая граница
    glutPrint(rightBorder,0,GLUT_BITMAP_HELVETICA_18, pr,0.0,0.0,0.0);  //правая граница
    //название
    if (functionToIntegration == func1) glutPrint((leftBorder+rightBorder)/2, MAX_Y, GLUT_BITMAP_HELVETICA_18, fun1,0.0,0.0,0.0);
    else if (functionToIntegration == func2) glutPrint((leftBorder+rightBorder)/2, MAX_Y, GLUT_BITMAP_HELVETICA_18, fun2,0.0,0.0,0.0);
    else if (functionToIntegration == func3) glutPrint((leftBorder+rightBorder)/2, MAX_Y, GLUT_BITMAP_HELVETICA_18, fun3,0.0,0.0,0.0);
    else glutPrint(0, MAX_Y, GLUT_BITMAP_HELVETICA_18, "???",0.0,0.0,0.0);

    glFlush();
}

void PrintMenu()
{
    cout << "1. О программе и методе Монте Карло\n";
    cout << "2. Выбрать функцию для интегрирования\n";
    cout << "3. Перейти к визуализации\n";
    cout << "4. Тест\n";
    cout << "5. Выход.\n";
}

void PrintInfo()
{
    ifstream INFO;//("info.txt");
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
        case 1: {functionToIntegration = func1; break;}
        case 2: {functionToIntegration = func2; break;}
        case 3: {functionToIntegration = func3; break;}
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
    //cout << "Введите количество итераций, точек и потоков через пробел:\n";
    //cin >> params[0] >> params[1] >> params[2];
    int params[] = {1000, 10000, 2};
    for (int i = 0; i < 50; i++, params[1] += 10000)
    {
        Test(params[0], params[1]);
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
            case 1: {PrintInfo(); break;}
            case 2: {ChooseFunction(); break;}
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
            case 4: {CreateTest(); break;}
            case 5: {start = true; break;}
            default: {cout << "Некорректный ввод. Попробуйте еще раз.\n";}
        }
    }

    return 0;
}
