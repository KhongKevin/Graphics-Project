/*
TEAM 9 Excavator.cpp
Kevin Khong, Matthew Lau, Cong Ho
*/

#include <glad.h>					// OpenGL access 
#include <glfw3.h>					// application framework
#include <time.h>
#include "Camera.h"					// view transforms from mouse input
#include "Draw.h"
#include "GLXtras.h"				// SetUniform
#include "Mesh.h"

// Window, camera, directory, colors
int		winWidth = 1000, winHeight = 1000;
Camera	camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -10));
vec3	org(1, .55f, 0), blk(0, 0, 0), yellow(.8f, .8f, 0.0f);

// Excavator
vec3	tracksCOR(0, 0.4f, 0);		// center of rotation (about Z-axis, for tracks and cab)
Mesh	excavatorTracks, excavatorLowerArm, excavatorUpperArm, excavatorCab, floorMesh;

// Lights
vec3	lights[] = { {1.3f, -.4f, .45f}, {-.4f, .6f, 1.f}, {.02f, .01f, .85f} };
const	int nLights = sizeof(lights) / sizeof(vec3);

// Mouse Interaction
Mover	mover;
void* picked = NULL;

// Keyboard Operations
int		key = 0, keyCount = 0;
time_t	keydownTime = 0;

void TestKey() {
	if (keyCount < 20) keyCount++;
	if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
		// rotate tracks
		float a = key == GLFW_KEY_LEFT ? 1.f : -1.f;
		excavatorTracks.toWorld = excavatorTracks.toWorld * Translate(tracksCOR) * RotateZ(a) * Translate(-tracksCOR);
	}
	if (key == 'K' || key == 'L') {
		// rotate cab
		float a = key == 'K' ? -1.f : 1.f;
		excavatorCab.toWorld = excavatorCab.toWorld * Translate(tracksCOR) * RotateZ(a) * Translate(-tracksCOR);
		excavatorCab.SetWrtParent();
	}
	if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
		// move tracks
		float speedMin = .005f, speedMax = .02f;
		float a = keyCount / 20.f, speed = speedMin + a * (speedMax - speedMin); // accelerate for 20 keystrokes
		float dy = key == GLFW_KEY_UP ? -speed : speed;
		excavatorTracks.toWorld = excavatorTracks.toWorld * Translate(0, dy, 0);
	}
	// recursively update matrices
	excavatorTracks.SetToWorld();
}

void CheckKey() {
	if (KeyDown(key) && (float)(clock() - keydownTime) / CLOCKS_PER_SEC > .1f)
		TestKey();
}

void Keyboard(GLFWwindow* w, int k, int scancode, int action, int mods) {
	if (action == GLFW_RELEASE)
		key = 0;
	if (action == GLFW_PRESS) {
		// cache key for subsequent test by CheckKey
		key = k;
		keydownTime = clock();
		keyCount = 0;
		TestKey();
	}
}

// Display

void Display() {
	glClearColor(.4f, .4f, .8f, 1);						// set background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// clear background and z-buffer
	glEnable(GL_DEPTH_TEST);							// see only nearest surface 
	int s = UseMeshShader();
	SetUniform(s, "facetedShading", true);
	SetUniform(s, "twoSidedShading", true);

	// Transform, update lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++)
		xLights[i] = Vec3(camera.modelview * vec4(lights[i]));
	SetUniform(s, "nLights", nLights);
	SetUniform3v(s, "lights", nLights, (float*)xLights);

	// Render the floor
	SetUniform(s, "color", vec3(.2f, .2f, .5f));
	floorMesh.Display(camera);

	// Render Excavator by parts
	SetUniform(s, "color", yellow);
	excavatorTracks.Display(camera);
	excavatorCab.Display(camera);
	excavatorLowerArm.Display(camera);
	excavatorUpperArm.Display(camera);

	// Render annotations
	UseDrawShader(camera.fullview);
	glDisable(GL_DEPTH_TEST);
	// Disk(centerOfRotation, 12, vec3(1,0,0));
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 6, org, blk);
	if (picked == &camera)
		camera.Draw();

	glFlush();
}

// Mouse Handlers

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (down) {
		for (size_t i = 0; i < nLights && !picked; i++)
			if (MouseOver(x, y, lights[i], camera.fullview)) {
				mover.Down(&lights[i], (int)x, (int)y, camera.modelview, camera.persp);
				picked = &mover;
			}
		if (!picked) {
			picked = &camera;
			camera.Down(x, y, Shift(), Control());
		}
	}
	else
		camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		if (picked == &mover)
			mover.Drag((int)x, (int)y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}

// Application

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

const char* usage = R"(
	mouse-drag:        rotate scene
	with shift:        translate scene xy
	mouse-wheel:       translate scene z
	up/down arrows:    drive forward/backward
	left/right arrows: rotate tracks
	K/L:               rotate cab
)";

int main(int ac, char** av) {
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "Excavation Celebration");

	// read excavator files
	string dir("C:/Users/Jules/Code/GG-Projects/#9-Excavator/");
	excavatorTracks.Read(dir + "tracks.obj", NULL, false);
	excavatorLowerArm.Read(dir + "lowerArmFinal.obj", NULL, false);
	excavatorUpperArm.Read(dir + "Arm.obj", NULL, false);
	excavatorCab.Read(dir + "cabFinal.obj", NULL, false);

	// position parts
	excavatorTracks.toWorld = Translate(0, 0, 0.1f);
	excavatorCab.toWorld = Translate(0, 0, 0.25f);
	excavatorUpperArm.toWorld = Translate(0, 0, 0.35f);
	excavatorLowerArm.toWorld = Translate(0, 0, 0.5f);

	// make mesh heirarchy, tracks as root with cab as child
	excavatorTracks.children.push_back(&excavatorCab);
	excavatorCab.parent = &excavatorTracks;
	excavatorCab.SetWrtParent();

	// cab has lower arm as child
	excavatorCab.children.push_back(&excavatorLowerArm);
	excavatorLowerArm.parent = &excavatorCab;
	excavatorLowerArm.SetWrtParent();

	// lower arm has upper arm as child
	excavatorLowerArm.children.push_back(&excavatorUpperArm);
	excavatorUpperArm.parent = &excavatorLowerArm;
	excavatorUpperArm.SetWrtParent();

	// Create the floor
	floorMesh.Read(dir + "flat_floor.obj");
	floorMesh.toWorld = Translate(0, 0, -.5f) * RotateX(-90) * Scale(2);

	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	glfwSetKeyCallback(w, Keyboard);
	printf("Usage:%s", usage);

	// event loop
	while (!glfwWindowShouldClose(w)) {
		CheckKey();
		Display();
		glfwPollEvents();
		glfwSwapBuffers(w);
	}

}
