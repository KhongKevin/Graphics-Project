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
Mesh	excavatorTracks, excavatorLowerArm, excavatorUpperArm, excavatorCab, floorMesh, ball;

int lowerArmRotation = 0;
int upperArmRotation = 0;


//location managment
vec3 centerOfRotation(0, 0.4f, 0);
//vec3 centerOfLowerArmRotation(0, 0.4f, 0);
vec3 centerOfLowerArmRotation(0, 0.1f, -.3f);
vec3 centerOfUpperArmRotation(0, -1.0f, .3f);
vec3 excavatorVelocity(0, 0, 0);
vec3 excavatorAcceleration(0, 0, 0);

vec3 ballPosition = vec3(0, 0, 0);

// Lights
vec3	lights[] = { {1.3f, -.4f, .45f}, {-.4f, .6f, 1.f}, {.02f, .01f, .85f} };
const	int nLights = sizeof(lights) / sizeof(vec3);

// Mouse Interaction
Mover	mover;
void* picked = NULL;

// Keyboard Operations
int		key = 0, keyCount = 0;
time_t	keydownTime = 0;

bool ballPickedUp = false; // a flag to indicate if the ball has been picked up

void PickupBall() {
	if (!ballPickedUp && key == 'B') {
		vec4 ballPosition = ball.toWorld * vec4(vec3(0, 0, 0), 1); // get the ball's position
		vec4 upperArmPosition = excavatorUpperArm.toWorld * vec4(vec3(0, 0, 0), 1); // get the upper arm's position
		float distance = length(vec3(ballPosition) - vec3(upperArmPosition)); // calculate the distance between them

		if (distance <= 0.2 && upperArmRotation <= 0) { // if the distance is less than or equal to a certain threshold
			excavatorUpperArm.children.push_back(&ball); // add the ball to the children of the upper arm
			ball.parent = &excavatorUpperArm; // set the parent of the ball to be the upper arm
			ball.SetWrtParent(); // update the ball's position relative to its parent
			ballPickedUp = true; // set the flag to true
		}
	}
}




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
	if (key == 'P' && (lowerArmRotation < 5)) {
		excavatorLowerArm.toWorld = excavatorLowerArm.toWorld * Translate(centerOfLowerArmRotation) * RotateX(7) * Translate(-centerOfLowerArmRotation);
		//excavatorLowerArm.toWorld = excavatorLowerArm.toWorld * Translate(0, -0.2f, 0);
		excavatorLowerArm.SetWrtParent();
		lowerArmRotation++;
	}
	if (key == 'O' && (lowerArmRotation > -5)) {
		excavatorLowerArm.toWorld = excavatorLowerArm.toWorld * Translate(centerOfLowerArmRotation) * RotateX(-7) * Translate(-centerOfLowerArmRotation);
		//excavatorLowerArm.toWorld = excavatorLowerArm.toWorld * Translate(0, 0.2f, 0);
		excavatorLowerArm.SetWrtParent();
		lowerArmRotation--;
	}

	if (key == 'G' && (upperArmRotation < 5)) {
		excavatorUpperArm.toWorld = excavatorUpperArm.toWorld * Translate(centerOfUpperArmRotation) * RotateX(7) * Translate(-centerOfUpperArmRotation);
		//excavatorLowerArm.toWorld = excavatorLowerArm.toWorld * Translate(0, -0.2f, 0);
		excavatorUpperArm.SetWrtParent();
		upperArmRotation++;
	}
	if (key == 'H' && (upperArmRotation > -5)) {
		excavatorUpperArm.toWorld = excavatorUpperArm.toWorld * Translate(centerOfUpperArmRotation) * RotateX(-7) * Translate(-centerOfUpperArmRotation);
		//excavatorLowerArm.toWorld = excavatorLowerArm.toWorld * Translate(0, 0.2f, 0);
		excavatorUpperArm.SetWrtParent();
		upperArmRotation--;

	}
	PickupBall();
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
	SetUniform(s, "color", vec3(0.2,0.2,0.2));
	excavatorTracks.Display(camera);
	SetUniform(s, "color", yellow);
	excavatorCab.Display(camera);
	excavatorLowerArm.Display(camera);
	excavatorUpperArm.Display(camera);

	// Render the ball
	SetUniform(s, "color", vec3(1.0f, 0.0f, 0.0f)); // Set the color of the ball
	ball.Display(camera); // The ball's toWorld transformation will be calculated automatically


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
	string dir("C:/Users/kevin/source/repos/Graphics-Project/Assets/Models/");
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
	floorMesh.Read(dir + "flat_floor.obj", NULL, false);
	floorMesh.toWorld = Translate(0, 0, -.5f) * RotateX(-90) * Scale(2);

	//Create the ball
	ball.Read(dir + "sphere.obj", NULL);
	ball.toWorld = Translate(0, 0, -0.2) * Scale(0.1);

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
