/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

// author: Ben Holmes

/*
 * This test uses the built-in glsl derivative functions (dFdx and dFdy)
 * on varying values.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "piglit-util.h"

static GLboolean Automatic = GL_FALSE;
static GLuint tex[1];
static GLint prog1;
static GLint vs1;
static GLint fs1;
static GLint prog2;
static GLint fs2;


static GLfloat verts[12] = {175.0, 125.0, 0.0,
				175.0, 175.0, 0.0,
				125.0, 125.0, 0.0,
				125.0, 175.0, 0.0};

static GLfloat texCoords[8] = {1.0, 0.0,
				1.0, 1.0,
				0.0, 0.0,
				0.0, 1.0};


static const char *vertShaderText =
	"attribute vec2 textureCoords;\n"
	"varying vec2 texCoords;\n"
	"void main()\n"
	"{ \n"
	"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
	"	texCoords = textureCoords;\n"
	"} \n";

static const char *fragShaderText =
	"uniform sampler2D tex2d;\n"
	"varying vec2 texCoords;\n"
	"void main()\n"
	"{ \n"
	"	gl_FragColor = texture2D(tex2d, texCoords);\n"
	"} \n";

static const char *fragShaderText2 =
	"uniform sampler2D tex2d;\n"
	"varying vec2 texCoords;\n"
	"void main()\n"
	"{ \n"
	"gl_FragColor = vec4(dFdx(texCoords.x), dFdy(texCoords.y),0.0,1.0);\n"
	"} \n";



static void
Init()
{

	glewInit();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 400, 0, 300, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glClearColor(0.6, 0.6, 0.6, 1.0);
}

static void
compileLinkProg()
{
	GLint stat;

	vs1 = glCreateShader(GL_VERTEX_SHADER);
	fs1 = glCreateShader(GL_FRAGMENT_SHADER);

	fs2 = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vs1, 1, (const GLchar **) &vertShaderText, NULL);
	glShaderSource(fs1, 1, (const GLchar **) &fragShaderText, NULL);
	glShaderSource(fs2, 1, (const GLchar **) &fragShaderText2, NULL);

	glCompileShader(vs1);
	glGetShaderiv(vs1, GL_COMPILE_STATUS, &stat);
	if (!stat) {
                printf("error compiling vertex shader1!\n");
                exit(1);
        }

	glCompileShader(fs1);
	glGetShaderiv(fs1, GL_COMPILE_STATUS, &stat);
	if (!stat) {
		printf("error compiling fragment shader1!\n");
		exit(1);
	}

	glCompileShader(fs2);
	glGetShaderiv(fs2, GL_COMPILE_STATUS, &stat);
	if (!stat) {
		printf("error compiling fragment shader2!\n");
		exit(1);
	}


	prog1 = glCreateProgram();
	glAttachShader(prog1, vs1);
	glAttachShader(prog1, fs1);
	glBindAttribLocation(prog1, 1, "textureCoords");
	glLinkProgram(prog1);
	glUseProgram(prog1);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat),
                                verts);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat),
				texCoords);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);


	prog2 = glCreateProgram();
	glAttachShader(prog2, vs1);
	glAttachShader(prog2, fs2);
	glBindAttribLocation(prog2, 1, "textureCoords");
	glLinkProgram(prog2);
	glUseProgram(prog2);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat),
                               verts);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat),
				texCoords);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

static void
loadTex()
{
	int height = 2;
	int width = 2;
	int i, j;

	GLfloat texData[width][height][4];
	for (i=0; i < width; ++i) {
		for (j=0; j < height; ++j) {
			if ((i+j) & 1) {
				texData[i][j][0] = 1;
				texData[i][j][1] = 0;
				texData[i][j][2] = 1;
				texData[i][j][3] = 0;
			}
			else {
				texData[i][j][0] = 0;
				texData[i][j][1] = 1;
				texData[i][j][2] = 0;
				texData[i][j][3] = 1;
			}
		}
	}

	glGenTextures(1, tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			GL_RGBA, GL_FLOAT, texData);

}


static void
display()
{

	glClear(GL_COLOR_BUFFER_BIT);

	glPushMatrix();

	glUseProgram(prog1);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glTranslatef(75.0, 0.0, 0.0);

	glUseProgram(prog2);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glPopMatrix();

	GLboolean pass = GL_TRUE;

	float mostlyBlack[3] = {0.019608, 0.019608, 0.0};
	float green[3] = {0, 1, 0};

	pass = pass && piglit_probe_pixel_rgb(132, 125, green);
	pass = pass && piglit_probe_pixel_rgb(205, 125, mostlyBlack);

	if(Automatic) {
		piglit_report_result(pass ? PIGLIT_SUCCESS : PIGLIT_FAILURE);
		exit(pass ? 0 : 1);
	}

	glFinish();
	glutSwapBuffers();

}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	if(argc==2 && !strncmp(argv[1], "-auto", 5))
		Automatic=GL_TRUE;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(400, 300);
	glutCreateWindow("glsl-deriv-varyings");
	glutDisplayFunc(display);
	glutKeyboardFunc(piglit_escape_exit_key);

	Init();

	compileLinkProg();

	loadTex();

	glutMainLoop();

	return 0;
}

