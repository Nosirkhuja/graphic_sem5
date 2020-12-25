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
// ����� ��� �������� ���������� ������
//==============================================================================
class CameraView
{
public:
    // ��������� ������ � ���������� �����������
    Vector3D cen;
    Vector3D eye;
    Vector3D up;

    // ��������� ������ � ����������� �����������
    float camPhi;
    float camTheta;
    float camRadius;

    // ��������� ��� �������� ���� ������
    const float kAngleStep;
    const float kRadiusStep;

public:
    // �����������
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

    // ����� ������� ��������� ������ �� ����������� �����������
    void GetCameraPos()
    {
	eye.x = cos(camPhi) * sin(camTheta) * camRadius;
	eye.y = cos(camTheta) * camRadius;
	eye.z = sin(camPhi) * sin(camTheta) * camRadius;
    }
} Camera;


//==============================================================================
// ��������������� ������� ��� ��������� CSG-�������
//==============================================================================
// ����� ��� ��������� ����� � ������ ���������
void DrawSphere()
{
    GLUquadricObj *obj = gluNewQuadric();
    gluSphere(obj, 1.0f, 100, 100);
}

// ����� ��� ��������� ����������� ����� ��� ���������
void DrawTinySphere()
{
    glPushMatrix();
    glTranslatef(0.0f, 0.4f, 0.8f);
    glScalef(0.8f, 0.8f, 0.8f);
    DrawSphere();
    glPopMatrix();
}

// ����� ��� ��������� ���� � ������ ���������
void DrawCube()
{
    glPushMatrix();
    glScalef(0.5f, 0.5f, 1.0f);
    glutSolidCube(2);
    glPopMatrix();
}
//==============================================================================
// ����� ��� ��������� CSG-�������
//==============================================================================
class CSGObjectDraw
{
private:
    const int kCsgCycle;

private:
    // ����� ��� ��������� ������� �� ������� ��� ���������
    // * draw_func --- ������� ��������� �������
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

    // ����� ��� ��������� ��������
    // * draw_func --- ������� ��������� �������
    void ClipObject(
	void(*draw_func)())
    {
	glCullFace(GL_FRONT);
	draw_func();
	glCullFace(GL_BACK);
    }
public:
    // �����������
    CSGObjectDraw():
	kCsgCycle(10000)
    { }

    // ����� ��� ������ ��������� CSG-�������
    void Draw()
    {
	// ��������� ��������� �������
	int curTime = glutGet(GLUT_ELAPSED_TIME);
	float t = 1.0f * (curTime % kCsgCycle) / kCsgCycle;
	float angle = t * 360;

	glPushMatrix();
	glRotatef(angle, 0, 1, 0);
	glTranslatef(-2.0f, 2.0f, -4.0f);

	// ����������� ��������� ���������
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);

	// ������
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
// ���� ������� ��� ��������� ��������
//==============================================================================

// ����a����� ��� ������ � ����������� �� ��������� ������
bool EdgesComparator(pair<Vector3D, int> &a, pair<Vector3D, int> &b)
{
    double dist_a = (a.first - Camera.eye) & (a.first - Camera.eye);
    double dist_b = (b.first - Camera.eye) & (b.first - Camera.eye);

    return dist_a > dist_b;
}

class MorphingDraw
{
private:
    // ��������� ������� �������
    const float kShellRadBig;
    const float kShellRadSmall;
    const float kShellHeight;
    const float kShellSpirals;

    // ��������� ������� �������
    const float kA;
    const float kN;
    const float kM;

    // ��������� ��������
    const int kMorphCycle;
    static const int kSize = 100;

    // ��� ��� �������
    Vector3D p1[kSize * kSize];
    Vector3D p2[kSize * kSize];
    Vector3D p3[kSize * kSize];
    Vector3D p4[kSize * kSize];
    // ������ ������
    
    pair<Vector3D, int> edges[kSize * kSize];

private:
    // ���������� ����� ������� �������
    Vector3D GetDotObject1(float u, float v)
    {
	float x = kShellRadBig * (1 - v / (2 * M_PI)) * cos(kShellSpirals * v)
	    * (1 + cos(u)) + kShellRadSmall * cos(kShellSpirals * v);
	float z = kShellRadBig * (1 - v / (2 * M_PI)) * sin(kShellSpirals * v)
	    * (1 + cos(u)) + kShellRadSmall * sin(kShellSpirals * v);
	float y = kShellHeight * v / (2 * M_PI) + kShellRadBig * (1 - v / (2 * M_PI)) * sin(u);
	return Vector3D(x, y, z);
    }

    // ���������� ����� ������� �������
    Vector3D GetDotObject2(float u, float v)
    {
	u *= 2;
	float x = (kA + cos(kN * u / 2.0f) * sin(v) - sin(kN * u / 2.0f) * sin(2 * v)) * cos(kM * u );
	float y = (kA + cos(kN * u / 2.0f) * sin(v) - sin(kN * u / 2.0f) * sin(2 * v)) * sin(kM * u );
	float z = sin(kN * u / 2.0f) * sin(v) + cos(kN * u / 2.0f) * sin(2 * v);
	return Vector3D(x, y, z);
    }

public:
    // �����������
    MorphingDraw():
	// �������������� ��������� ������� �������
	kShellRadBig(0.8f),
	kShellRadSmall(0.1f),
	kShellHeight(5.0f),
	kShellSpirals(2),
	// �������������� ��������� ������� �������
	kA(2),
	kN(2),
	kM(1),
	// �������������� ��������� ��������
	kMorphCycle(4000)
    { }

    // ����� ��� ��������� ������� ��������
    void Draw()
    {
	glDepthFunc(GL_LEQUAL);

	glPushMatrix();
	glTranslatef(1.0f, 4.5f, 0.0f);

	// ��������� � ����� ������� ���� �������
	float t;
	int curTime = glutGet(GLUT_ELAPSED_TIME);
	if ((curTime % (2 * kMorphCycle)) < kMorphCycle)
	    t = 1.0f * (curTime % kMorphCycle) / kMorphCycle;
	else
	    t = 1 - 1.0f * (curTime % kMorphCycle) / kMorphCycle;

	// ������� ��� ������� � ������ ������
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

	// ����������� ����� �� ��������� �����������
	sort(edges, edges + kSize * kSize, EdgesComparator);
	
	// ������ ������
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
// ����� ��� ��������� NURBS-�����������
//==============================================================================
class NURBSObject
{
private:
    static const int kNURBSSize = 5;

    GLUnurbsObj *obj;
    float cPoints[kNURBSSize][kNURBSSize][3];

public:
    // �����������
    NURBSObject()
    {
	// ������� ����������� �����
	for (int x = 0; x < kNURBSSize; x++) {
	    for (int z = 0; z < kNURBSSize; z++) {
		cPoints[x][z][0] = x;
		cPoints[x][z][1] = rand() % 2 ? 1.0f * rand() / RAND_MAX : -1.0f * rand() / RAND_MAX;
		cPoints[x][z][2] = z;
	    }
	}

	// �������������� NURBS-������
	obj = gluNewNurbsRenderer();
	gluNurbsProperty(obj, GLU_CULLING, GL_TRUE);
	gluNurbsProperty(obj, GLU_DISPLAY_MODE, GLU_FILL);
    }

    // ����� ��� ��������� NURBS-�����������
    void Draw()
    {
	glEnable(GL_AUTO_NORMAL);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	// ������� ����������� �����
	float kn[] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };

	// �������� ��������� �����������
	glPushMatrix();
	glScalef(5, 1, 5);
	glTranslatef(-2.0f, 0.0f, -2.0f);

	glColor4ub(0x99, 0x66, 0x99, 150);

	// ������
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
// ���� ������� ��� ��������� ����� �� ���������
//==============================================================================
class BillboardDraw
{
private:

    const float kHDrops;
    static const int kSize = 20;

    // ���������� ���������
    pair <Vector3D, float> drops[kSize][kSize];
    // ����� ���������
    pair<Vector3D, int> edges[kSize * kSize];

private:
    // ����� ��� ������� �������� �� �����������
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
    // �����������
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

    // ����� ��� �������� �����
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

    // ����� ��� ��������� ����� �� ���������
    void Draw()
    {
	// �������� ������ ������
	for (int i = 0; i < kSize; i++)
	    for (int j = 0; j < kSize; ++j)
		edges[kSize * i + j] = make_pair(drops[i][j].first, kSize * i + j);

	// ����������� ����� �� ��������� �����������
	sort(edges, edges + kSize * kSize, EdgesComparator);
	glColor4ub(255, 255, 255, 100);

	// �������� ���������
	for (int i = 0; i < kSize * kSize; ++i) {
	    Vector3D coords = drops[edges[i].second / kSize][edges[i].second % kSize].first;

	    glPushMatrix();
	    glTranslatef(coords.x, coords.y, coords.z);
	    glScalef(0.16f, 0.18f, 0.16f);

	    // ��������� ������� �� �����������
	    AdjustObject(Camera.eye, coords);

	    // ������ ��������
	    glBegin(GL_QUADS);
	    glNormal3f(0.0f, 1.0f, 0.0f);
	    glVertex3f(0.0f, 0.0f, 0.0f);
	    glVertex3f(0.0f, 0.0f, 1.0f);
	    glVertex3f(1.0f, 0.0f, 1.0f);
	    glVertex3f(1.0f, 0.0f, 0.0f);
	    glEnd();

	    // ������ ��������� ������� ��� ��������
	    glPopMatrix();

	    glPopMatrix();
	}
    }

} BillboardObject;

//==============================================================================



// ������� ��� ��������� �����
void RenderScene()
{
    NURBSSurface.Draw();

    CSGObject.Draw();

    BillboardObject.Animate();
    BillboardObject.Draw();

    MorphingObject.Draw();
}



//==============================================================================
// �������� ��� glut-�
// DisplayFunc() --- ������� ��� Display-�������, ������ ���� ����
// IdleFunc --- ������� ��� Idle-�������, ������ ��������� ����������� ��� �������� FPS
// InitFunc --- ������� ��� ������������� ���� �������� � �����
// ReshapeFunc --- ������� ����������� ���� � ����������� �� ����� � ������
// KeyboardFunc --- ������� ��������� ������� � ����������
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

    glEnable(GL_FOG);                       // �������� ����� (GL_FOG)
    glFogi(GL_FOG_MODE, GL_EXP2);	    // �������� ��� ������
    glFogfv(GL_FOG_COLOR, fogcolor);        // ������������� ���� ������
    glFogf(GL_FOG_DENSITY, 0.05f);	    // ��������� ������ ����� �����
    glHint(GL_FOG_HINT, GL_DONT_CARE);      // ��������������� ��������� ������
    glFogf(GL_FOG_START, 0.1f);             // �������, � ������� ���������� �����
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

    // �������� ���������
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_AMBIENT, Vector4D(0.6f, 0.6f, 0.6f, 1));
    glLightfv(GL_LIGHT0, GL_DIFFUSE, Vector4D(0.2f, 0.2f, 0.2f, 1));
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 4);
    glLighti(GL_LIGHT0, GL_SPOT_CUTOFF, 80);
    glEnable(GL_LIGHT0);

    // �������� ���������
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
    // ESC --- �����
    if (key == '\033')
	exit(0);

    // w/W --- �������� ������ �� ��������� �����
    if (key == 'w' || key == 'W') {
	Camera.camTheta-= Camera.kAngleStep;
	if (Camera.camTheta < 0.0f)
	    Camera.camTheta = 0.01f;
    }
    // s/S --- �������� ������ �� ��������� ����
    if (key == 's' || key == 'S') {
	Camera.camTheta += Camera.kAngleStep;
	if (Camera.camTheta > M_PI)
	    Camera.camTheta = M_PI - 0.01f;
    }
    // a/A --- �������� ������ �� ��������� �����
    if (key == 'a' || key == 'A') {
	Camera.camPhi -= Camera.kAngleStep;
	if (Camera.camPhi < 0.0f)
	    Camera.camPhi += 2 * M_PI;
    }
    // a/A --- �������� ������ �� ��������� ������
    if (key == 'd' || key == 'D') {
	Camera.camPhi += Camera.kAngleStep;
	if (Camera.camPhi > 2 * M_PI)
	    Camera.camPhi -= 2 * M_PI;
    }
    // q/Q --- ����������� ������ � ������
    if (key == 'q' || key == 'Q') {
	Camera.camRadius -= Camera.kRadiusStep;
	if (Camera.camRadius < 0.0f)
	    Camera.camRadius = 0.1f;
    }
    // e/E--- ��������� ������ �� ������
    if (key == 'e' || key == 'E') {
	Camera.camRadius += Camera.kRadiusStep;
    }

    // �������� ���������� ��������� ������, ����� ��������� ������������ �����
    Camera.GetCameraPos();
}
//==============================================================================

int main(
    int argc, char* argv[])
{
    int windowWidth = GetSystemMetrics(SM_CXSCREEN);
    int windowHeight = GetSystemMetrics(SM_CYSCREEN);

    // �������������� glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_STENCIL | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    int window = glutCreateWindow("");
    glutFullScreen();

    // �������� ��������
    InitFunc();
    glutDisplayFunc(DisplayFunc);
    glutIdleFunc(IdleFunc);
    glutReshapeFunc(ReshapeFunc);
    glutKeyboardFunc(KeyboardFunc);

    // ����������� ���� glut
    glutMainLoop();
    glutDestroyWindow(window);
    return 0;
}

