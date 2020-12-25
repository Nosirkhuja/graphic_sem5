#pragma comment(lib, "Glu32.lib")
#pragma comment(lib, "Glut32.lib")
#pragma comment(lib, "Opengl32.lib")

#include "gl.h"
#include "glut.h"

#include "Vector2D.h"
#include "Vector3D.h"
#include "Vector4D.h"

#include <iostream>
#include <algorithm>
#include <cstdio>

using namespace std;


//==============================================================================
// Класс для хранения параметров камеры
//==============================================================================
class CameraView
{
public:
    // Параметры камеры в декартовых координатах
    Vector3D cen;
    Vector3D eye;
    Vector3D up;

    // Параметры камеры в сферических координатах
    float camPhi;
    float camTheta;
    float camRadius;

    // Константы для скорости шага камеры
    const float kAngleStep;
    const float kRadiusStep;

public:
    // Конструктор
    CameraView():
	kAngleStep(M_PI / 40),
	kRadiusStep(0.3f)
    {
	cen = Vector3D(0.0f, 0.0f, 0.0f);
	eye = Vector3D(2.0f, 2.0f, 2.0f);
	up = Vector3D(0.0f, 1.0f, 0.0f);

	camPhi = M_PI / 4;
	camTheta = M_PI / 4;
	camRadius = 20.0f;
    }

    // Метод расчета положения камеры по сферическим координатам
    void GetCameraPos()
    {
	eye.x = cos(camPhi) * sin(camTheta) * camRadius;
	eye.y = cos(camTheta) * camRadius;
	eye.z = sin(camPhi) * sin(camTheta) * camRadius;
    }
} Camera;


//==============================================================================
// Вспомогательные функции для отрисовки CSG-объекта
//==============================================================================
// Метод для отрисовки сферы в начале координат
void DrawSphere()
{
    GLUquadricObj *obj = gluNewQuadric();
    gluSphere(obj, 1.0f, 100, 100);
}

// Метод для отрисовки уменьшенной сферы для вычитания
void DrawTinySphere()
{
    glPushMatrix();
    glTranslatef(0.0f, 0.4f, 0.8f);
    glScalef(0.8f, 0.8f, 0.8f);
    DrawSphere();
    glPopMatrix();
}

// Метод для отрисовки куба в начале координат
void DrawCube()
{
    glPushMatrix();
    glScalef(0.5f, 0.5f, 1.0f);
    glutSolidCube(2);
    glPopMatrix();
}
//==============================================================================
// Класс для отрисовки CSG-объекта
//==============================================================================
class CSGObjectDraw
{
private:
    const int kCsgCycle;

private:
    // Метод для вычитания объекта из другого при отрисовке
    // * draw_func --- функция отрисовки объекта
    void SubstractObject(
	void(*draw_func)())
    {
	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearStencil(0);
	glCullFace(GL_BACK);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	glStencilFunc(GL_ALWAYS, 0, 0);

	draw_func();

	glCullFace(GL_FRONT);
	glDepthMask(GL_TRUE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_NOTEQUAL, 0, 1);
	glDepthFunc(GL_GREATER);

	draw_func();

	glDisable(GL_STENCIL_TEST);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
    }

    // Метод для наложения объектов
    // * draw_func --- функция отрисовки объекта
    void ClipObject(
	void(*draw_func)())
    {
	glCullFace(GL_FRONT);
	draw_func();
	glCullFace(GL_BACK);
    }
public:
    // Конструктор
    CSGObjectDraw():
	kCsgCycle(10000)
    { }

    // Метод для полной отрисовки CSG-объекта
    void Draw()
    {
	// Вычисляем положение объекта
	int curTime = glutGet(GLUT_ELAPSED_TIME);
	float t = 1.0f * (curTime % kCsgCycle) / kCsgCycle;
	float angle = t * 360;

	glPushMatrix();
	glRotatef(angle, 0, 1, 0);
	glTranslatef(-2.0f, 2.0f, -4.0f);

	// Настраиваем параметры отрисовки
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);

	// Рисуем
	DrawSphere();
	SubstractObject(DrawCube);
	SubstractObject(DrawTinySphere);
	ClipObject(DrawSphere);

	glDepthFunc(GL_EQUAL);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glCullFace(GL_BACK);
	glColor3ub(0, 154, 205);
	DrawSphere();

	glCullFace(GL_FRONT);
	glColor3ub(138, 43, 226);
	DrawCube();
	glColor3ub(255, 105, 180);
	DrawTinySphere();

	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);
	glPopMatrix();
    }
} CSGObject;
//==============================================================================



//==============================================================================
// Блок функций для отрисовки морфинга
//==============================================================================

// Компaратор для граней в зависимости от положения камеры
bool EdgesComparator(pair<Vector3D, int> &a, pair<Vector3D, int> &b)
{
    double dist_a = (a.first - Camera.eye) & (a.first - Camera.eye);
    double dist_b = (b.first - Camera.eye) & (b.first - Camera.eye);

    return dist_a > dist_b;
}

class MorphingDraw
{
private:
    // Параметры первого объекта
    const float kShellRadBig;
    const float kShellRadSmall;
    const float kShellHeight;
    const float kShellSpirals;

    // Параметры второго объекта
    const float kA;
    const float kN;
    const float kM;

    // Параметры морфинга
    const int kMorphCycle;
    static const int kSize = 100;

    // Меш для объекта
    Vector3D p1[kSize * kSize];
    Vector3D p2[kSize * kSize];
    Vector3D p3[kSize * kSize];
    Vector3D p4[kSize * kSize];
    // Массив граней
    
    pair<Vector3D, int> edges[kSize * kSize];

private:
    // Координаты точек первого объекта
    Vector3D GetDotObject1(float u, float v)
    {
	float x = kShellRadBig * (1 - v / (2 * M_PI)) * cos(kShellSpirals * v)
	    * (1 + cos(u)) + kShellRadSmall * cos(kShellSpirals * v);
	float z = kShellRadBig * (1 - v / (2 * M_PI)) * sin(kShellSpirals * v)
	    * (1 + cos(u)) + kShellRadSmall * sin(kShellSpirals * v);
	float y = kShellHeight * v / (2 * M_PI) + kShellRadBig * (1 - v / (2 * M_PI)) * sin(u);
	return Vector3D(x, y, z);
    }

    // Координаты точек второго объекта
    Vector3D GetDotObject2(float u, float v)
    {
	u *= 2;
	float x = (kA + cos(kN * u / 2.0f) * sin(v) - sin(kN * u / 2.0f) * sin(2 * v)) * cos(kM * u );
	float y = (kA + cos(kN * u / 2.0f) * sin(v) - sin(kN * u / 2.0f) * sin(2 * v)) * sin(kM * u );
	float z = sin(kN * u / 2.0f) * sin(v) + cos(kN * u / 2.0f) * sin(2 * v);
	return Vector3D(x, y, z);
    }

public:
    // Конструктор
    MorphingDraw():
	// Инициализируем параметры первого объекта
	kShellRadBig(0.8f),
	kShellRadSmall(0.1f),
	kShellHeight(5.0f),
	kShellSpirals(2),
	// Инициализируем параметры второго объекта
	kA(2),
	kN(2),
	kM(1),
	// Инициализируем параметры морфинга
	kMorphCycle(4000)
    { }

    // Метод для отрисовки объекта морфинга
    void Draw()
    {
	glDepthFunc(GL_LEQUAL);

	glPushMatrix();
	glTranslatef(1.0f, 4.5f, 0.0f);

	// Посчитаем в какую сторону идет морфинг
	float t;
	int curTime = glutGet(GLUT_ELAPSED_TIME);
	if ((curTime % (2 * kMorphCycle)) < kMorphCycle)
	    t = 1.0f * (curTime % kMorphCycle) / kMorphCycle;
	else
	    t = 1 - 1.0f * (curTime % kMorphCycle) / kMorphCycle;

	// Обновим меш объекта и массив граней
	int p = 0;
	float kStep = 2 * M_PI / kSize;
	for (int i = 0; i < kSize; i++) {
	    for (int j = 0; j < kSize; j++) {
		float u = i * kStep;
		float v = j * kStep;
		p1[p] = t * GetDotObject1(u, v) + (1 - t) * GetDotObject2(u, v);
		p2[p] = t * GetDotObject1(u, v + kStep) + (1 - t) * GetDotObject2(u, v + kStep);
		p3[p] = t * GetDotObject1(u + kStep, v + kStep) + (1 - t) * GetDotObject2(u + kStep, v + kStep);
		p4[p] = t * GetDotObject1(u + kStep, v) + (1 - t) * GetDotObject2(u + kStep, v);

		edges[p].first = t * GetDotObject1(u + kStep / 2, v + kStep / 2) + (1 - t) * GetDotObject2(u + kStep / 2, v + kStep / 2);
		edges[p].second = p++;
	    }
	}

	// Отсортируем грани по положению наблюдателя
	sort(edges, edges + kSize * kSize, EdgesComparator);
	
	// Рисуем объект
	glColor4ub(30, 144, 255, 150);
	glBegin(GL_QUADS);
	for (int i = 0; i < kSize * kSize; i++) {
	    glVertex3fv(p1[edges[i].second]);
	    glVertex3fv(p2[edges[i].second]);
	    glVertex3fv(p3[edges[i].second]);
	    glVertex3fv(p4[edges[i].second]);
	}

	glEnd();

	glPopMatrix();
	glDepthFunc(GL_LEQUAL);
    }

} MorphingObject;
//==============================================================================



//==============================================================================
// Класс для отрисовки NURBS-поверхности
//==============================================================================
class NURBSObject
{
private:
    static const int kNURBSSize = 5;

    GLUnurbsObj *obj;
    float cPoints[kNURBSSize][kNURBSSize][3];

public:
    // Конструктор
    NURBSObject()
    {
	// Зададим контрольные точки
	for (int x = 0; x < kNURBSSize; x++) {
	    for (int z = 0; z < kNURBSSize; z++) {
		cPoints[x][z][0] = x;
		cPoints[x][z][1] = rand() % 2 ? 1.0f * rand() / RAND_MAX : -1.0f * rand() / RAND_MAX;
		cPoints[x][z][2] = z;
	    }
	}

	// Инициализируем NURBS-объект
	obj = gluNewNurbsRenderer();
	gluNurbsProperty(obj, GLU_CULLING, GL_TRUE);
	gluNurbsProperty(obj, GLU_DISPLAY_MODE, GLU_FILL);
    }

    // Метод для отрисовки NURBS-поверхности
    void Draw()
    {
	glEnable(GL_AUTO_NORMAL);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	// Выберем контрольные точки
	float kn[] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };

	// Настроим положение поверхности
	glPushMatrix();
	glScalef(5, 1, 5);
	glTranslatef(-2.0f, 0.0f, -2.0f);

	glColor4ub(0x99, 0x66, 0x99, 150);

	// Рисуем
	gluBeginSurface(obj);
	gluNurbsSurface(
	    obj,
	    10, kn,
	    10, kn,
	    15, 3, 
	    &cPoints[0][0][0], kNURBSSize, kNURBSSize, GL_MAP2_VERTEX_3);
	gluEndSurface(obj);

	glPopMatrix();

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	glDisable(GL_AUTO_NORMAL);
    }

} NURBSSurface;
//==============================================================================

//==============================================================================
// Блок функций для отрисовки дождя из билбордов
//==============================================================================
class BillboardDraw
{
private:

    const float kHDrops;
    static const int kSize = 20;

    // Координаты билбордов
    pair <Vector3D, float> drops[kSize][kSize];
    // Грани билбордов
    pair<Vector3D, int> edges[kSize * kSize];

private:
    // Метод для поворта билборда на наблюдателя
    void AdjustObject(Vector3D eye, Vector3D pos)
    {
	Vector3D lookat, objincam, up;
	float phicos;

	glPushMatrix();
	objincam[0] = eye.x - pos.x;
	objincam[1] = 0;
	objincam[2] = eye.z - pos.z;

	lookat[0] = 0;
	lookat[1] = 1;
	lookat[2] = 0;

	objincam.normalize();
	up = lookat ^ objincam;

	phicos = lookat & objincam;

	if ((phicos < 0.9999) && (phicos > -0.9999))
	    glRotatef(acos(phicos) * 180 / M_PI, up[0], up[1], up[2]);
    }

public:
    // Конструктор
    BillboardDraw():
	kHDrops(7.0f)
    {
	for (int i = 0; i < kSize; ++i) {
	    for (int j = 0; j < kSize; ++j) {
		drops[i][j].first = Vector3D(
		    (i - kSize / 2) / 0.5f + 0.5f * rand() / RAND_MAX,
		    kHDrops + 20 * rand() / RAND_MAX,
		    (j - kSize / 2) / 0.5f + 0.5f * rand() / RAND_MAX);
		drops[i][j].second = 0.006f + 0.008f * rand() / RAND_MAX;
	    }
	}
    }

    // Метод для анимации дождя
    void Animate()
    {
	for (int i = 0; i < kSize; ++i) {
	    for (int j = 0; j < kSize; ++j) {
		drops[i][j].first.y -= drops[i][j].second;
		if (drops[i][j].first.y < 0)
		    drops[i][j].first.y = kHDrops + 20 * rand() / RAND_MAX;
	    }
	}
    }

    // Метод для отрисовки дождя из билбордов
    void Draw()
    {
	// Заполним массив граней
	for (int i = 0; i < kSize; i++)
	    for (int j = 0; j < kSize; ++j)
		edges[kSize * i + j] = make_pair(drops[i][j].first, kSize * i + j);

	// Отсортируем грани по положению наблюдателя
	sort(edges, edges + kSize * kSize, EdgesComparator);
	glColor4ub(255, 255, 255, 100);

	// Нарисуем билбиорды
	for (int i = 0; i < kSize * kSize; ++i) {
	    Vector3D coords = drops[edges[i].second / kSize][edges[i].second % kSize].first;

	    glPushMatrix();
	    glTranslatef(coords.x, coords.y, coords.z);
	    glScalef(0.16f, 0.18f, 0.16f);

	    // Поверенем билборд на наблюдателя
	    AdjustObject(Camera.eye, coords);

	    // Рисуем дождинку
	    glBegin(GL_QUADS);
	    glNormal3f(0.0f, 1.0f, 0.0f);
	    glVertex3f(0.0f, 0.0f, 0.0f);
	    glVertex3f(0.0f, 0.0f, 1.0f);
	    glVertex3f(1.0f, 0.0f, 1.0f);
	    glVertex3f(1.0f, 0.0f, 0.0f);
	    glEnd();

	    // Вернем параметры обратно для билборда
	    glPopMatrix();

	    glPopMatrix();
	}
    }

} BillboardObject;

//==============================================================================



// Функция для отрисовки сцены
void RenderScene()
{
    NURBSSurface.Draw();

    CSGObject.Draw();

    BillboardObject.Animate();
    BillboardObject.Draw();

    MorphingObject.Draw();
}



//==============================================================================
// Коллбеки для glut-а
// DisplayFunc() --- коллбек для Display-функции, рисует один кадр
// IdleFunc --- Коллбек для Idle-функции, просто запускаем перерисовку для большего FPS
// InitFunc --- Функция для инициализации всех объектов и сцены
// ReshapeFunc --- Функция перерисовки окна в зависимости от длины и ширины
// KeyboardFunc --- Функция обработки событий с клавиатуры
//==============================================================================
void DisplayFunc()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0xCC, 0xCC, 0x99, 1);

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gluLookAt(
	Camera.eye.x, Camera.eye.y, Camera.eye.z,
	Camera.cen.x, Camera.cen.y, Camera.cen.z,
	Camera.up.x, Camera.up.y, Camera.up.z);

    glLightfv(GL_LIGHT0, GL_POSITION, Vector4D(0, 10, 0, 1));
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, Vector4D(0, -1, 0, 1));

#if 0
    float fogcolor[3] =  { 0.6f, 0.6f, 0.6f};

    glEnable(GL_FOG);                       // Включает туман (GL_FOG)
    glFogi(GL_FOG_MODE, GL_EXP2);	    // Выбираем тип тумана
    glFogfv(GL_FOG_COLOR, fogcolor);        // Устанавливаем цвет тумана
    glFogf(GL_FOG_DENSITY, 0.05f);	    // Насколько густым будет туман
    glHint(GL_FOG_HINT, GL_DONT_CARE);      // Вспомогательная установка тумана
    glFogf(GL_FOG_START, 0.1f);             // Глубина, с которой начинается туман
    glFogf(GL_FOG_END, 5.0f);
#endif

    RenderScene();

    glutSwapBuffers();
}

void IdleFunc()
{
    glutPostRedisplay();
}

void InitFunc()
{
    Camera.GetCameraPos();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearDepth(1);

    // Настроим освещение
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_AMBIENT, Vector4D(0.6f, 0.6f, 0.6f, 1));
    glLightfv(GL_LIGHT0, GL_DIFFUSE, Vector4D(0.2f, 0.2f, 0.2f, 1));
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 4);
    glLighti(GL_LIGHT0, GL_SPOT_CUTOFF, 80);
    glEnable(GL_LIGHT0);

    // Настроим материалы
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Vector4D(0.7f, 0.7f, 0.7f, 1));
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Vector4D(0.8f, 0.8f, 0.8f, 1));
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Vector4D(0.3f, 0.3f, 0.3f, 1));
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 25);
}

void ReshapeFunc(
    int w, int h)
{
    if (w != 0) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLdouble)(w / (GLdouble)h), 0.1, 100.0);
    }
}

void KeyboardFunc(
    unsigned char key,
    int mx, int my)
{
    // ESC --- выход
    if (key == '\033')
	exit(0);

    // w/W --- движение камеры по меридиану вверх
    if (key == 'w' || key == 'W') {
	Camera.camTheta-= Camera.kAngleStep;
	if (Camera.camTheta < 0.0f)
	    Camera.camTheta = 0.01f;
    }
    // s/S --- движение камеры по меридиану вниз
    if (key == 's' || key == 'S') {
	Camera.camTheta += Camera.kAngleStep;
	if (Camera.camTheta > M_PI)
	    Camera.camTheta = M_PI - 0.01f;
    }
    // a/A --- движение камеры по параллели влево
    if (key == 'a' || key == 'A') {
	Camera.camPhi -= Camera.kAngleStep;
	if (Camera.camPhi < 0.0f)
	    Camera.camPhi += 2 * M_PI;
    }
    // a/A --- движение камеры по параллели вправо
    if (key == 'd' || key == 'D') {
	Camera.camPhi += Camera.kAngleStep;
	if (Camera.camPhi > 2 * M_PI)
	    Camera.camPhi -= 2 * M_PI;
    }
    // q/Q --- приближение камеры к центру
    if (key == 'q' || key == 'Q') {
	Camera.camRadius -= Camera.kRadiusStep;
	if (Camera.camRadius < 0.0f)
	    Camera.camRadius = 0.1f;
    }
    // e/E--- отдаление камеры от центра
    if (key == 'e' || key == 'E') {
	Camera.camRadius += Camera.kRadiusStep;
    }

    // Запомним измененное положение камеры, чтобы корректно перерисовать сцену
    Camera.GetCameraPos();
}
//==============================================================================

int main(
    int argc, char* argv[])
{
    int windowWidth = GetSystemMetrics(SM_CXSCREEN);
    int windowHeight = GetSystemMetrics(SM_CYSCREEN);

    // Инициализируем glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_STENCIL | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    int window = glutCreateWindow("");
    glutFullScreen();

    // Настроим коллбеки
    InitFunc();
    glutDisplayFunc(DisplayFunc);
    glutIdleFunc(IdleFunc);
    glutReshapeFunc(ReshapeFunc);
    glutKeyboardFunc(KeyboardFunc);

    // Бесконечный цикл glut
    glutMainLoop();
    glutDestroyWindow(window);
    return 0;
}

