/*
TEAM 9 Excavator.cpp
Kevin Khong, Matthew Lau, Cong Ho

*/
/*
#include <cmath>										// for sin, cos
#include <glad.h>										// OpenGL access 
#include <glfw3.h>										// application framework
#include <time.h>
#include "Camera.h"										// view transforms on mouse input
#include "Draw.h"
#include "GLXtras.h"									// SetUniform
#include "Mesh.h"										// cube.Read
#include "Misc.h"										// Shift

#define M_PI 3.14159265358979323846

// Window, camera, directory, colors
int winWidth = 1000, winHeight = 1000;
Camera camera(0, 0, winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -10), 30, .001f, 500);
string dir("D:/kevin/Code/Apps/Assets/Models/");
vec3 org(1, .55f, 0), blk(0, 0, 0);

// Excavator
vec3 centerOfRotation(0, 0.4f, 0);
vec3 excavatorVelocity(0, 0, 0);
vec3 excavatorAcceleration(0, 0, 0);
float maxVelocity = 0.02f;
float maxAcceleration = 0.05f;


//Meshes
Mesh excavatorTracks, excavatorLowerArm, excavatorUpperArm, excavatorCab, floorMesh;


// Lights
vec3 lights[] = { {1.3f, -.4f, .45f}, {-.4f, .6f, 1.f}, {.02f, .01f, .85f} };
const int nLights = sizeof(lights) / sizeof(vec3);

// Interaction
Mover mover;
void* picked = NULL;


vec3 clamp(const vec3& v, float minVal, float maxVal) {
	vec3 clamped;
	for (int i = 0; i < 3; i++)
		clamped[i] = v[i] < minVal ? minVal : v[i] > maxVal ? maxVal : v[i];
	return clamped;
}

void AccelerateExcavator(vec3 inputAcceleration, float damping = 1.0f) {
	// Gradually increase acceleration up to maxAcceleration
	for (int i = 0; i < 3; i++) {
		if (abs(excavatorAcceleration[i] + inputAcceleration[i]) <= maxAcceleration) {
			excavatorAcceleration[i] += inputAcceleration[i];
		}
		else {
			excavatorAcceleration[i] = (excavatorAcceleration[i] > 0 ? maxAcceleration : -maxAcceleration);
		}
	}

	// Update velocity based on acceleration
	excavatorVelocity = clamp(excavatorVelocity + excavatorAcceleration, -maxVelocity, maxVelocity);

	// Update the position based on the velocity
	mat4 translationMatrix = Translate(excavatorVelocity.x, excavatorVelocity.y, excavatorVelocity.z);
	//excavator.toWorld = excavator.toWorld * translationMatrix;


	excavatorTracks.toWorld = translationMatrix * excavatorTracks.toWorld;
	excavatorTracks.SetToWorld();

	// Reset the acceleration
	excavatorAcceleration = vec3(0, 0, 0);

	// Apply damping to the velocity
	excavatorVelocity *= damping;
}

//void Key(GLFWwindow* w, int key, int scancode, int action, int mods) {
//	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
//		vec3 acceleration(0, 0, 0);
//		switch (key) {
//		case GLFW_KEY_UP:
//			acceleration.y = -0.005f; // accelerate excavator forward
//			break;
//		case GLFW_KEY_DOWN:
//			acceleration.y = 0.005f; // accelerate excavator backward
//			break;
//		case GLFW_KEY_RIGHT:
//			acceleration.x = -0.005f; // accelerate excavator left
//			break;
//		case GLFW_KEY_LEFT:
//			acceleration.x = 0.005f; // accelerate excavator right
//			break;
//		case 'R':
//			excavatorTracks.toWorld = excavatorTracks.toWorld * Translate(centerOfRotation) * RotateZ(10) * Translate(-centerOfRotation);
//			break;
//		case 'E':
//			excavatorTracks.toWorld = excavatorTracks.toWorld * Translate(centerOfRotation) * RotateZ(-10) * Translate(-centerOfRotation);
//			break;
//		}
//		//excavatorTracks.SetToWorld();
//		AccelerateExcavator(acceleration);
//	}
//}

void Key(int key, bool press, bool shift, bool control) {
	if (press) {
		vec3 acceleration(0, 0, 0);
		if(key == GLFW_KEY_UP) acceleration.y = -0.005f; // accelerate excavator forward
		if(key == GLFW_KEY_DOWN) acceleration.y = 0.005f; // accelerate excavator backward
		if(key == GLFW_KEY_RIGHT)acceleration.x = -0.005f; // accelerate excavator left
		if(key == GLFW_KEY_LEFT)acceleration.x = 0.005f; // accelerate excavator right
		if(key == 'R')
			excavatorTracks.toWorld = excavatorTracks.toWorld * Translate(centerOfRotation) * RotateZ(10) * Translate(-centerOfRotation);
		if(key == 'E')
			excavatorTracks.toWorld = excavatorTracks.toWorld * Translate(centerOfRotation) * RotateZ(-10) * Translate(-centerOfRotation);
		if (key == 'K') {
			excavatorCab.toWorld = excavatorCab.toWorld * Translate(centerOfRotation) * RotateZ(10) * Translate(-centerOfRotation);
			excavatorCab.SetWrtParent();
		}

		//excavatorTracks.SetToWorld();
		AccelerateExcavator(acceleration);
	}
}

void Display() {
	glClearColor(.4f, .4f, .8f, 1);						// set background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// clear background and z-buffer
	glEnable(GL_DEPTH_TEST);							// see only nearest surface 
	int s = UseMeshShader();
	SetUniform(s, "color", vec3(.2f, .2f, .5f));
	SetUniform(s, "facetedShading", true);
	SetUniform(s, "twoSidedShading", true);

	// Transform, update lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++)
		xLights[i] = Vec3(camera.modelview * vec4(lights[i]));
	SetUniform(s, "nLights", nLights);
	SetUniform3v(s, "lights", nLights, (float*)xLights);

	// Render the floor
	floorMesh.Display(camera);

	// Render the Excavator
	SetUniform(s, "color", vec3(.8f, .8f, 0.0f));

	//Render Excavator by parts
	excavatorTracks.Display(camera, 0);
	excavatorCab.Display(camera, 0);
	excavatorLowerArm.Display(camera, 0);
	excavatorUpperArm.Display(camera, 0);


	// Render annotations
	UseDrawShader(camera.fullview);
	glDisable(GL_DEPTH_TEST);
	//	Disk(centerOfRotation, 12, vec3(1,0,0));
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 6, org, blk);
	if (picked == &camera)
		camera.Draw();

	glFlush();											// finish
}

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
	if (!down)
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

void MouseWheel(float spin) { camera.Wheel(spin, Shift()); }

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char** av) {
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "3D-Mesh");


	// Load in excavator by parts
	excavatorTracks.Read(dir + "tracks.obj", dir + "yellow.jpg", NULL, false);
	excavatorLowerArm.Read(dir + "lowerArmFinal.obj", dir + "yellow.jpg", NULL, false);
	excavatorUpperArm.Read(dir + "Arm.obj", dir + "yellow.jpg", NULL, false);
	excavatorCab.Read(dir + "cabFinal.obj", dir + "yellow.jpg", NULL, false);

	// Assemble Parts
	excavatorTracks.toWorld = Translate(0, 0, 0.1f);
	excavatorCab.toWorld = Translate(0, 0, 0.25f);
	excavatorUpperArm.toWorld = Translate(0, 0, 0.35f);
	excavatorLowerArm.toWorld = Translate(0, 0, 0.5f);

	excavatorTracks.children.push_back(&excavatorCab);
	excavatorCab.parent = &excavatorTracks;
	excavatorCab.SetWrtParent();

	excavatorCab.children.push_back(&excavatorLowerArm);
	excavatorLowerArm.parent = &excavatorCab;
	excavatorLowerArm.SetWrtParent();

	excavatorLowerArm.children.push_back(&excavatorUpperArm);
	excavatorUpperArm.parent = &excavatorLowerArm;
	excavatorUpperArm.SetWrtParent();

	// Create the floor mesh
	floorMesh.Read(dir + "flat_floor.obj");
	floorMesh.toWorld = Translate(0, 0, -.5f) * RotateX(-90) * Scale(2);


	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	RegisterKeyboard(Key);
	printf("mouse-drag:  rotate\nwith shift:  translate xy\nmouse-wheel: translate z\narrowkeys: translate x,y");
	// event loop
	//glfwSetKeyCallback(w, Key);


	while (!glfwWindowShouldClose(w)) {
		//AccelerateExcavator(vec3(0, 0, 0), 0.9f);
		Display();
		glfwPollEvents();
		glfwSwapBuffers(w);
	}

}
*/

//For Future Camera Controls and or excavator movement. Uses Arrow Keys as input.
//void Key(GLFWwindow* w, int key, int scancode, int action, int mods) {
//	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
//		vec3 displacement(0, 0, 0);
//		switch (key) {
//		case GLFW_KEY_UP:
//			displacement.z = -0.1f; // move camera forward
//			break;
//		case GLFW_KEY_DOWN:
//			displacement.z = 0.1f; // move camera backward
//			break;
//		case GLFW_KEY_LEFT:
//			displacement.x = -0.1f; // move camera left
//			break;
//		case GLFW_KEY_RIGHT:
//			displacement.x = 0.1f; // move camera right
//			break;
//		}
//		camera.Move(displacement); // move the camera based on the displacement
//	}
//}