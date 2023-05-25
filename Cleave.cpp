// Cleave: display movable/orientable quad as a plane to cleave a mesh

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <time.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "Mesh.h"
#include "Widgets.h"

// display
int		winW = 600, winH = 600;
Camera	camera(0, 0, winW, winH, vec3(20, 20, 0), vec3(0, 0, -5));
bool	useCleaverDisplay = true;

// colors
vec3	wht(1, 1, 1), red(1, 0, 0), grn(0, 1, 0), blu(0, 0, 1), yel(1, 1, 0), mag(1, 0, 1);

// lighting
vec3	light(-.2f, .4f, .3f);

// interaction
Mover	mover;								// position light
Framer	framer;								// position/orient mesh
void* picked = NULL;						// &mover or &framer
time_t	mouseEvent = clock();

// scene
Mesh	cleaver, mesh, yin, yang;
bool	cleaved = false;

// directories
string	assets = "D:/kevin/Code/Apps/Assets", objDir = assets + "Models/", imgDir = assets + "Images/";

// Geometry

vec4 Plane(mat4 quadToWorld) {
	vec3 z = Vec3(quadToWorld * vec4(0, 0, 1, 0));
	vec3 o = Vec3(quadToWorld * vec4(0, 0, 0, 1));
	// compute plane given three points
	return vec4(z.x, z.y, z.z, -dot(o, z));
}

void CopyVertices(Mesh& from, Mesh& to) {
	int npoints = from.points.size(), nnormals = from.normals.size(), nuvs = from.uvs.size();
	to.points.resize(npoints);
	to.normals.resize(nnormals);
	to.uvs.resize(nuvs);
	for (int i = 0; i < npoints; i++) to.points[i] = from.points[i];
	for (int i = 0; i < nnormals; i++) to.normals[i] = from.normals[i];
	for (int i = 0; i < nuvs; i++) to.uvs[i] = from.uvs[i];
	to.triangles.resize(0);
	to.quads.resize(0);
}

bool Side(vec3 p, vec4 plane) { return dot(plane, vec4(p, 1)) < 0; }

bool FullyIn(vec4 plane, Mesh& m, int3 t) {
	vec3 v1 = m.points[t.i1], v2 = m.points[t.i2], v3 = m.points[t.i3];
	return Side(v1, plane) && Side(v2, plane) && Side(v3, plane);
}

bool FullyIn(vec4 plane, Mesh& m, int4 q) {
	vec3 v1 = m.points[q.i1], v2 = m.points[q.i2], v3 = m.points[q.i3], v4 = m.points[q.i4];
	return Side(v1, plane) && Side(v2, plane) && Side(v3, plane) && Side(v4, plane);
}

void Condense(Mesh& m) {
	int npoints = m.points.size(), count = 0;
	// ignoring uvs and normals, for now
	vector<int> map(npoints, -1);
	vector<vec3> temp(npoints);
	for (int i = 0; i < npoints; i++)
		temp[i] = m.points[i];
	for (size_t i = 0; i < m.triangles.size(); i++) {
		int3& t = m.triangles[i];
		if (map[t.i1] == -1) { m.points[count] = temp[t.i1]; map[t.i1] = count; t.i1 = count++; }
		else t.i1 = map[t.i1];
		if (map[t.i2] == -1) { m.points[count] = temp[t.i2]; map[t.i2] = count; t.i2 = count++; }
		else t.i2 = map[t.i2];
		if (map[t.i3] == -1) { m.points[count] = temp[t.i3]; map[t.i3] = count; t.i3 = count++; }
		else t.i3 = map[t.i3];
	}
	for (size_t i = 0; i < m.quads.size(); i++) {
		int4& q = m.quads[i];
		if (map[q.i1] == -1) { m.points[count] = temp[q.i1]; map[q.i1] = count; q.i1 = count++; }
		else q.i1 = map[q.i1];
		if (map[q.i2] == -1) { m.points[count] = temp[q.i2]; map[q.i2] = count; q.i2 = count++; }
		else q.i2 = map[q.i2];
		if (map[q.i3] == -1) { m.points[count] = temp[q.i3]; map[q.i3] = count; q.i3 = count++; }
		else q.i3 = map[q.i3];
		if (map[q.i4] == -1) { m.points[count] = temp[q.i4]; map[q.i4] = count; q.i4 = count++; }
		else q.i4 = map[q.i4];
	}
	m.points.resize(count);
}

void Cleave() {
	vec4 plane = Plane(cleaver.toWorld);
	vec3 offset = .25f * normalize(vec3(plane.x, plane.y, plane.z));
	CopyVertices(mesh, yin);
	CopyVertices(mesh, yang);
	for (size_t i = 0; i < mesh.triangles.size(); i++) {
		Mesh* m = FullyIn(plane, mesh, mesh.triangles[i]) ? &yin : &yang;
		m->triangles.push_back(mesh.triangles[i]);
	}
	for (size_t i = 0; i < mesh.quads.size(); i++) {
		Mesh* m = FullyIn(plane, mesh, mesh.quads[i]) ? &yin : &yang;
		m->quads.push_back(mesh.quads[i]);
	}
	Condense(yin);
	Condense(yang);
	printf("yin %i triangles, %i quads, %i vertices\n", yin.triangles.size(), yin.quads.size(), yin.points.size());
	printf("yang %i triangles, %i quads, %i vertices\n", yang.triangles.size(), yang.quads.size(), yang.points.size());
	yin.Buffer();
	yang.Buffer();
	yin.textureName = yang.textureName = mesh.textureName;
	yin.toWorld = Translate(-offset) * mesh.toWorld;
	yang.toWorld = Translate(offset) * mesh.toWorld;
	cleaved = true;
}

// Display

void Outline(mat4 quadToWorld) {
	vec3 pts[] = { {-1,-1,0}, {1,-1,0}, {1,1,0}, {-1,1,0} }, xpts[4];
	for (int i = 0; i < 4; i++)
		xpts[i] = Vec3(quadToWorld * vec4(pts[i], 1));
	Quad(xpts[0], xpts[1], xpts[2], xpts[3], false, red, 1, 2);
}

void Display() {
	// clear screen, depth test, blend
	vec3 c = wht;//blu;
	glClearColor(c.x, c.y, c.z, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	// update light
	int s = UseMeshShader();
	vec3 xlight = Vec3(camera.modelview * vec4(light, 1));
	SetUniform(s, "nLights", 1);
	SetUniform3v(s, "lights", 1, (float*)&xlight);
	// display objects
	SetUniform(s, "opacity", 1);
	SetUniform(s, "useCleaver", useCleaverDisplay);
	vec4 plane = Plane(camera.modelview * cleaver.toWorld);
	SetUniform(s, "cleaver", plane);
	if (cleaved) {
		yin.Display(camera, 0);
		yang.Display(camera, 0);
	}
	else mesh.Display(camera, 0);
	SetUniform(s, "useCleaver", false);
	SetUniform(s, "opacity", .5f);
	SetUniform(s, "color", blu);
	cleaver.Display(camera);
	UseDrawShader(camera.fullview);
	Outline(cleaver.toWorld);
	// lights and frames
	if ((clock() - mouseEvent) / CLOCKS_PER_SEC < 1.f) {
		glDisable(GL_DEPTH_TEST);
		Disk(light, 9, vec3(1, 1, 0));
		mat4& f = cleaver.toWorld;
		vec3 base(f[0][3], f[1][3], f[2][3]);
		Disk(base, 9, vec3(1, 1, 1));
		if (picked == &framer)
			framer.Draw(camera.fullview);
		if (picked == &camera)
			camera.arcball.Draw();
	}
	glFlush();
}

// Mouse

void MouseButton(float x, float y, bool left, bool down) {
	if (left && down) {
		int ix = (int)x, iy = (int)y;
		void* newPicked = NULL;
		// hit light?
		if (ScreenD(x, y, light, camera.fullview) < 12) {
			newPicked = &mover;
			mover.Down(&light, ix, iy, camera.modelview, camera.persp);
		}
		if (!newPicked) {
			// hit cleaver base?
			vec3 base(cleaver.toWorld[0][3], cleaver.toWorld[1][3], cleaver.toWorld[2][3]);
			if (MouseOver(x, y, base, camera.fullview)) {
				newPicked = &framer;
				framer.Set(&cleaver.toWorld, 100, camera.fullview);
				framer.Down(ix, iy, camera.modelview, camera.persp);
			}
		}
		if (!newPicked && picked == &framer && framer.Hit(ix, iy)) {
			framer.Down(ix, iy, camera.modelview, camera.persp);
			newPicked = &framer;
		}
		picked = newPicked;
		if (!picked) {
			picked = &camera;
			camera.Down(x, y);
		}
	}
	if (!down && picked == &camera)
		camera.Up();
	if (!down && picked == &framer)
		framer.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	mouseEvent = clock();
	if (leftDown) {
		int ix = (int)x, iy = (int)y;
		if (picked == &mover)
			mover.Drag(ix, iy, camera.modelview, camera.persp);
		if (picked == &framer)
			framer.Drag(ix, iy, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	if (picked == &framer)
		framer.Wheel(spin, Shift());
	if (picked == &camera)
		camera.Wheel(spin, Shift());
}

// Application

string GetInput(const char* prompt) {
	char buf[500];
	printf("%s: ", prompt);
	fgets(buf, 500, stdin);
	if (strlen(buf) > 1) buf[strlen(buf) - 1] = 0; // remove <cr>
	return string(buf);
}

string dir;

void Keyboard(int key, bool press, bool shift, bool control) {
	if (!press) return;
	if (key == 'N') {
		string name = GetInput("file name (complete path): ");
		const char* n = name.c_str(), * s = strrchr(n, '/');
		char buf[500];
		int len = s - n + 1;
		strncpy(buf, n, len);
		buf[len] = 0;
		dir = string(buf);
		printf("dir = %s\n", dir.c_str());
		mesh.Read(name);
	}
	if (key == 'S') {
		string yinName = dir + GetInput("half-1 name (no path): ");
		string yangName = dir + GetInput("half-2 name (no path): ");
		WriteAsciiObj(yinName.c_str(), yin.points, yin.normals, yin.uvs, &yin.triangles, &yin.quads);
		WriteAsciiObj(yangName.c_str(), yang.points, yang.normals, yang.uvs, &yang.triangles, &yang.quads);
		printf("%s and %s written\n", yinName.c_str(), yangName.c_str());
	}
	if (key == 'D') useCleaverDisplay = !useCleaverDisplay;
	if (key == 'C') Cleave();
	if (key == 'R') cleaved = false;
}

void Resize(int width, int height) {
	glViewport(0, 0, winW = width, winH = height);
	camera.Resize(width, height);
}

const char* usage = R"(
	N: new mesh
	C: cleave
	D: toggle cleaver display
	R: reset
	S: save
)";

//"D:/kevin/Code/Apps/Assets/excavator_mesh.obj"
//D:/kevin/Code/Apps/Assets/Models/lowerArmFinal.obj
int main(int ac, char** av) {
	// init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winW, winH, "Leave it to Cleaver");
	// make scene
	// mesh.Read(objDir+"Bench.obj", imgDir+"Bench.tga");s
	//0.35f
	mesh.toWorld = Translate(0, 0, 0);
	cleaver.Read(objDir + "Quad.obj"); // in xy-plane, normal = z-axis
	//xyz
	cleaver.toWorld = Translate(0, 0, -0.15f) * RotateY(-90);
	printf("Usage%s\n", usage);
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterKeyboard(Keyboard);
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &mesh.vBufferId);
	glfwDestroyWindow(w);
	glfwTerminate();
}
