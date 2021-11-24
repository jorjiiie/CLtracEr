#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>


typedef unsigned char uchar;

int main() {
	GLFWwindow* window;

	if (!glfwInit()) {
		printf("couldn't init window\n");
		return 1;
	}

	window = glfwCreateWindow(700,100,"hi\n",NULL,NULL);
	if (!window) {
		printf("no window\n");
		return 1;
	}
	uchar* data = (uchar*) malloc(sizeof(uchar) * 100 * 100 * 3);
	for (int i=0;i<100;i++) {
		for (int j=0;j<100;j++) {
			data[i*100*3 + j*3 ] = 0xff;
			data[i*100*3 + j*3 +1] = 0x00;
			data[i*100*3 + j*3 +2] = 0x00;
		}
	}
	glfwMakeContextCurrent(window);
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawPixels(100,100,GL_RGB,GL_UNSIGNED_BYTE,data);
		glfwSwapBuffers(window);
		glfwWaitEvents();
	}
	return 0;
}