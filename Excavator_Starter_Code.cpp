/*
TEAM 9 Starter Code
Kevin Khong, Matthew Lau, Cong Ho

*/

#include <glad.h>										// OpenGL access 
#include <glfw3.h>										// application framework
#include "Camera.h"										// view transforms on mouse input
#include "GLXtras.h"									// SetUniform
#include "Mesh.h"										// cube.Read
#include "Misc.h"										// Shift
#include <cmath> // for sin, cos
#define M_PI 3.14159265358979323846

int winWidth = 700, winHeight = 700;
Camera camera(0, 0, winWidth, winHeight, vec3(0,0,0), vec3(0,0,-10), 30, .001f, 500);

Mesh excavator;
Mesh floorMesh;

const char* floorMeshFile = "D:/kevin/Code/Apps/Assets/flat_floor.obj";
const char* excavatorMeshFile = "D:/kevin/Code/Apps/Assets/FabConvert.com_uploads_files_4292192_excavator+caterpillar+323d+-.obj";

//NOT WORKING (Need to find online mesh files with textures)
const char *textureFile = "D:/kevin/Code/Apps/Assets/Parrots.jpg";
const char* yellowTextureFile = "D:/kevin/Code/Apps/Assets/yellow.jpg";


//For Future Camera Controls and or excavator movement. Uses Arrow Keys as input.
void Key(GLFWwindow* w, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		vec3 displacement(0, 0, 0);
		switch (key) {
		case GLFW_KEY_UP:
			displacement.z = -0.1f; // move camera forward
			break;
		case GLFW_KEY_DOWN:
			displacement.z = 0.1f; // move camera backward
			break;
		case GLFW_KEY_LEFT:
			displacement.x = -0.1f; // move camera left
			break;
		case GLFW_KEY_RIGHT:
			displacement.x = 0.1f; // move camera right
			break;
		}
		camera.Move(displacement); // move the camera based on the displacement
	}
}

// Rotation matrix around axis
mat4 rotate(const mat4& m, float angle, const vec3& axis) {
	float c = std::cos(angle);
	float s = std::sin(angle);
	vec3 a = normalize(axis);
	vec3 temp = (1.0f - c) * a;

	mat4 rot(1.0f);
	rot[0][0] = c + temp.x * a.x;
	rot[0][1] = temp.x * a.y + s * a.z;
	rot[0][2] = temp.x * a.z - s * a.y;
	rot[1][0] = temp.y * a.x - s * a.z;
	rot[1][1] = c + temp.y * a.y;
	rot[1][2] = temp.y * a.z + s * a.x;
	rot[2][0] = temp.z * a.x + s * a.y;
	rot[2][1] = temp.z * a.y - s * a.x;
	rot[2][2] = c + temp.z * a.z;

	return m * rot;
}

// Convert degrees to radians
float radians(float degrees) {
	return degrees * static_cast<float>(M_PI) / 180.0f;
}


void Display(GLFWwindow *w) {
	glClearColor(.5f, .1f, .5f, 1);						// set background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// clear background and z-buffer
	glEnable(GL_DEPTH_TEST);							// see only nearest surface
	SetUniform(UseMeshShader(), "useLight", false);		// disable shading
	
	// Render the floor
	floorMesh.Display(camera, 0);
	excavator.Display(camera, 0, true);					// draw mesh with camera transform
	glFlush();											// finish
}



void MouseButton(float x, float y, bool left, bool down) {
	if (down) camera.Down(x, y, Shift()); else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) camera.Drag(x, y);
}

void MouseWheel(float spin) { camera.Wheel(spin, Shift()); }

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "3D-Mesh");
	excavator.Read(string(excavatorMeshFile), string(yellowTextureFile));

	// Create the floor mesh
	floorMesh.Read(string(floorMeshFile), string(yellowTextureFile)); 
	

	//make floor bigger
	mat4 scalingMatrix = Scale(2.0f);
	floorMesh.toWorld = scalingMatrix * floorMesh.toWorld;

	// rotate it to match excavator orientation
	mat4 rotateMat = rotate(mat4(1.0f), radians(90.0f), vec3(0, 1, 0));
	floorMesh.toWorld = floorMesh.toWorld * rotateMat;
	floorMesh.toWorld = rotate(mat4(1.0f), radians(-90.0f), vec3(1, 0, 0)) * floorMesh.toWorld;
	
	// translate the mesh 10 units down
	mat4 translationMatrix = Translate(0, 0, -0.5);
	floorMesh.toWorld = translationMatrix * floorMesh.toWorld;
	



	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	printf("mouse-drag:  rotate\nwith shift:  translate xy\nmouse-wheel: translate z\narrowkeys: translate x,y");
	// event loop
	glfwSwapInterval(1);
	glfwSetKeyCallback(w, Key);

	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwPollEvents();
		glfwSwapBuffers(w);
	}
}



