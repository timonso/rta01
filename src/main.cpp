// This file reuses code from my work for the Computer Graphics class assignments, including parts from the template file for Lab 4 of that class

#include <iostream>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <stack>
#include <cstdlib>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>

using namespace std;
using namespace glm;
using vec4 = glm::vec4;
using vec3 = glm::vec3;
using vec2 = glm::vec2;
using mat4 = glm::mat4;

static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode){};

typedef mat4(*InstantiateFn)();

// define the default camera properties, with 'direction' moving along the local y-axis
struct Camera {
	vec3 position = vec3(0.0f, 0.0f, 20.0f);
	vec3 target = vec3(0.0f, 0.0f, 0.0f);
	vec3 right = vec3(1.0f, 0.0f, 0.0f);
	vec3 up = vec3(0.0f, 1.0f, 0.0f);
	vec3 direction = vec3(0.0f, 0.0f, -1.0f);
	GLfloat pitch = 0.0f;
	GLfloat yaw = 0.0f;
	GLfloat translateSpeed = 1.0f;
	GLfloat rotateSpeed = 0.5f;
	GLfloat maxRotation = 60.0f;
};

struct MaterialData {
	// std::string name = "material";
	GLuint* shader;
	vec4 baseColor = vec4(0.9f, 0.9f, 0.9f, 1.0f);
	GLfloat alpha = 1.0f;
	vec4 specularColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	GLfloat specularCoeff = 0.2f;
	GLfloat phongExp = 100.0f;
};

struct MeshData {
	int vao;
	unsigned int materialIndex;
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
};

struct ModelParams {
	const char* path;
	std::vector<InstantiateFn> instantiateFns;
	const vector<MaterialData> materials;
};

struct ModelData {
	std::vector<InstantiateFn> instantiateFns;
	std::vector<MeshData> submeshes;
	std::vector<MaterialData> materials;
};

GLuint phongShader;
std::vector<GLuint*> shaders = { &phongShader };

// window dimensions
int width = 1200;
int height = 700;
int center_x = int(width / 2);
int center_y = int(height / 2);

// a stack that allows children models to access model matrices from their parent models
static std::stack<mat4> hierarchyStack;

// animation parameters
GLfloat fan_rotate_y = 0.0f;

vec3 up_global = vec3(0.0f, 1.0f, 0.0f);

std::vector<ModelData> models;
Camera camera;

MaterialData matPhong_01 = {
	// .shader = &phongShader,
	.specularCoeff = 0.2f,
	.phongExp = 5.0f,
};

static mat4 renderFan_left_01() {
	mat4 model = mat4(1.0f);
	model = glm::rotate(model, glm::radians(fan_rotate_y), vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(90.0f), vec3(1, 0, 0));
	model = glm::translate(model, vec3(-4.0f, 4.0f, 0));
	return model;
}

const std::string texturePath = "textures/";
vec4 lightPosition = vec4(-10.0f, 10.0f, 10.0f, 1.0f);

// define model objects and their instances
static std::vector<ModelParams> modelPaths = {
	{"../meshes/fan.obj", {renderFan_left_01}, {matPhong_01}},
};

#pragma region MESH LOADING

//int createTexture(const char* path) {
//	int width;
//	int height;
//	int channelCount;
//	unsigned char* pixels = stbi_load(path, &width, &height, &channelCount, 0);
//	unsigned int textureHandle;
//
//	glGenTextures(1, &textureHandle);
//	glBindTexture(GL_TEXTURE_2D, textureHandle);
//
//	// for JPG textures
//	GLint channelDepth = GL_RGB;
//	if (channelCount == 4) {
//		// for PNG textures with alpha channel
//		channelDepth = GL_RGBA;
//	}
//	glTexImage2D(GL_TEXTURE_2D, 0, channelDepth, width, height, 0, channelDepth, GL_UNSIGNED_BYTE, pixels);
//	glGenerateMipmap(GL_TEXTURE_2D);
//
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//	stbi_image_free(pixels);
//	return textureHandle;
//}

ModelData read_model(const char* file_name) {
	ModelData modelData;
	const aiScene* scene = aiImportFile(
			file_name,
			aiProcess_Triangulate | aiProcess_PreTransformVertices
		);

		if (!scene) {
			fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
			return modelData;
		}

		//printf("  %i materials\n", scene->mNumMaterials);
		//printf("  %i meshes\n", scene->mNumMeshes);
		//printf("  %i textures\n", scene->mNumTextures);

		for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
			aiMesh* mesh = scene->mMeshes[m_i];
			//printf("    %i vertices in mesh\n", mesh->mNumVertices);
			MeshData submesh;
			submesh.mPointCount = mesh->mNumVertices;
			submesh.materialIndex = mesh->mMaterialIndex;

			for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
				if (mesh->HasPositions()) {
					const aiVector3D* vp = &(mesh->mVertices[v_i]);
					submesh.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
				}
				if (mesh->HasNormals()) {
					const aiVector3D* vn = &(mesh->mNormals[v_i]);
					submesh.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
				}
				if (mesh->HasTextureCoords(0)) {
					const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
					submesh.mTextureCoords.push_back(vec2(vt->x, vt->y));
				}
				if (mesh->HasTangentsAndBitangents()) {
				}
			}
			modelData.submeshes.push_back(submesh);
		}

		// load materials
		for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
			const aiMaterial* aiMaterial = scene->mMaterials[i];
			MaterialData material;

			// aiString name;
			// aiMaterial->Get(AI_MATKEY_NAME, name);
			// material.name = name.C_Str();

			material.shader = &phongShader;

			GLfloat sExp;
			aiMaterial->Get(AI_MATKEY_SHININESS, sExp);
			material.specularCoeff = sExp;

			aiColor4D sColor;
			aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, sColor);
			material.specularColor = vec4(sColor.r, sColor.g, sColor.b, sColor.a);

			modelData.materials.push_back(material);
		}

		aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp = fopen(shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	glCompileShader(ShaderObj);
	GLint success;

	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
	}

	glAttachShader(ShaderProgram, ShaderObj);
}

void CompileShaders()
{
	phongShader = glCreateProgram();

	if (phongShader == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
	}

	AddShader(phongShader, "../shaders/baseVert.glsl", GL_VERTEX_SHADER);
	AddShader(phongShader, "../shaders/phongFrag.glsl", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };

	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glLinkProgram(phongShader);
	glGetProgramiv(phongShader, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(phongShader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking base shader program: " << ErrorLog << std::endl;
	}

	glValidateProgram(phongShader);
	glGetProgramiv(phongShader, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(phongShader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid phong shader program: " << ErrorLog << std::endl;
	}

	glBindVertexArray(0);
}
#pragma endregion SHADER_FUNCTIONS

#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh() {
	for (int i = 0; i < modelPaths.size(); i++) {
		ModelParams currentModel = modelPaths[i];
		ModelData modelData = read_model(currentModel.path);
		std::vector<MeshData>& submeshes = modelData.submeshes;
		modelData.instantiateFns = currentModel.instantiateFns;
		modelData.materials = currentModel.materials;

		// process all meshes in each loaded model
		for (int j = 0; j < submeshes.size(); j++) {
			MeshData& currentMesh = submeshes[j];
			unsigned int vao = (i + j) * 2;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			unsigned int vp_vbo = vao;
			glGenBuffers(1, &vp_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
			glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec3), &currentMesh.mVertices[0], GL_STATIC_DRAW);

			unsigned int vn_vbo = vao;
			glGenBuffers(1, &vn_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
			glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec3), &currentMesh.mNormals[0], GL_STATIC_DRAW);

			unsigned int vt_vbo = vao;
			glGenBuffers(1, &vt_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
			glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec2), &currentMesh.mTextureCoords[0], GL_STATIC_DRAW);

			GLuint vertPosLoc, vertNormalLoc, vertTexLoc;
			vertPosLoc = glGetAttribLocation(phongShader, "vertex_position");
			vertNormalLoc = glGetAttribLocation(phongShader, "vertex_normal");
			vertTexLoc = glGetAttribLocation(phongShader, "vertex_texture");

			glEnableVertexAttribArray(vertPosLoc);
			glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
			glVertexAttribPointer(vertPosLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

			glEnableVertexAttribArray(vertNormalLoc);
			glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
			glVertexAttribPointer(vertNormalLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

			glEnableVertexAttribArray(vertTexLoc);
			glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
			glVertexAttribPointer(vertTexLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);

			// save VAO reference in mesh properties
			currentMesh.vao = vao;
		}
		models.push_back(modelData);
	}
}
#pragma endregion VBO_FUNCTIONS

// write all material properties to the shader
void setMaterial(MaterialData material, mat4 locMat) {
	GLuint currentShader = phongShader;
	glUseProgram(currentShader);

	int modelMatLoc = glGetUniformLocation(currentShader, "model");
	glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, &locMat[0][0]);

	GLint baseColorLoc = glGetUniformLocation(currentShader, "baseColor");
	glUniform4fv(baseColorLoc, 1, &material.baseColor[0]);

	GLint alphaLoc = glGetUniformLocation(currentShader, "alpha");
	glUniform1f(alphaLoc, material.alpha);

	GLint specularColorLoc = glGetUniformLocation(currentShader, "specularColor");
	glUniform4fv(specularColorLoc, 1, &material.specularColor[0]);

	GLint phongExpLoc = glGetUniformLocation(currentShader, "phongExp");
	glUniform1f(phongExpLoc, material.phongExp);

	GLint specularCoeffLoc = glGetUniformLocation(currentShader, "specularCoeff");
	glUniform1f(specularCoeffLoc, material.specularCoeff);
}

void display() {
	vec4 backgroundColor = vec4(0.1f, 0.1f, 0.1f, 1.0f);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 model = mat4(1.0f);
	mat4 view = glm::lookAt(camera.position, camera.target, camera.up);
	mat4 persp_proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);

	vec4 LightPositionView = view * model * lightPosition;

	for (int i = 0; i < shaders.size(); i++) {
		GLuint currentShader = *shaders[i];
		glUseProgram(currentShader);
		int viewMatLoc = glGetUniformLocation(currentShader, "view");
		int projMatLoc = glGetUniformLocation(currentShader, "proj");

		int LightPosLoc = glGetUniformLocation(currentShader, "lightPosition");
		glUniform3fv(LightPosLoc, 1, &LightPositionView[0]);

		glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, &persp_proj[0][0]);
		glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, &view[0][0]);
	}

	// instantiate all submeshes of each model
	for (int i = 0; i < models.size(); i++) {
		ModelData currentModel = models[i];
		int numInstances = currentModel.instantiateFns.size();
		std::vector<MeshData> submeshes = currentModel.submeshes;
		// instantiate and draw meshes with their assigned materials
		for (int k = 0; k < numInstances; k++) {
			mat4 currentLocMat = currentModel.instantiateFns[k]();
			for (int j = 0; j < submeshes.size(); j++) {
				MeshData& currentMesh = submeshes[j];
				MaterialData currentMaterial = currentModel.materials[currentMesh.materialIndex];
				setMaterial(currentMaterial, currentLocMat);
				glBindVertexArray(currentMesh.vao);
				glDrawArrays(GL_TRIANGLES, 0, currentMesh.mPointCount);
			}
		}
	}
	glBindVertexArray(0);
	// glutSwapBuffers();
}


void updateScene() {
	static double last_time = 0;

	double curr_time = glfwGetTime();
	if (last_time == 0)
		last_time = curr_time;
	double delta = (curr_time - last_time);
	last_time = curr_time;

	fan_rotate_y += 40.0f * delta;
	fan_rotate_y = fmodf(fan_rotate_y, 360.0f);
}

void init()
{
	CompileShaders();
	generateObjectBufferMesh();
}

void keypress(unsigned char key, int x, int y) {
	if (key == 'w') {
		camera.position += camera.direction * camera.translateSpeed;
	}
	if (key == 's') {
		camera.position -= camera.direction * camera.translateSpeed;
	}
	if (key == 'q') {
		camera.position += camera.up * camera.translateSpeed;
	}
	if (key == 'e') {
		camera.position -= camera.up * camera.translateSpeed;
	}
	if (key == 'd') {
		camera.position += camera.right * camera.translateSpeed;
	}
	if (key == 'a') {
		camera.position -= camera.right * camera.translateSpeed;
	}
	// exit program on pressing 'Esc'
	if (key == 27) {
		exit(0);
	}

	// constrain the camera position to a certain volume in the scene
	// camera.position.v[0] = min(max(camera.position.v[0], -60.0f), 60.0f);
	// camera.position.v[1] = min(max(camera.position.v[1], -40.0f), 40.0f);
	// camera.position.v[2] = min(max(camera.position.v[2], -40.0f), 150.0f);

	// update the point the camera looks at by slightly moving away from
	// the camera position along the camera view direction
	camera.target = camera.position + camera.direction;
}

void mouseMove(int curr_x, int curr_y) {
	static int last_x = curr_x;
	static int last_y = curr_y;

	// calculate mouse position delta
	float delta_x = (curr_x - last_x) * camera.rotateSpeed;
	float delta_y = (curr_y - last_y) * camera.rotateSpeed;

	float newYaw = camera.yaw + delta_x;
	float newPitch = camera.pitch + delta_y;

	// translate to origin
	mat4 cameraMat = mat4(1.0f);
	cameraMat = glm::translate(cameraMat, -camera.direction);

	// rotate according to the deltas
	cameraMat = glm::rotate(cameraMat, glm::radians(-delta_x), vec3(0, 1, 0));

	// constrain camera pitch
	if (abs(newPitch) < camera.maxRotation) {
		cameraMat = glm::rotate(cameraMat, glm::radians(-delta_y), vec3(1, 0, 0));
		camera.yaw = newYaw;
		camera.pitch = newPitch;
	}

	// reapply initial translation
	cameraMat = glm::translate(cameraMat, camera.direction);

	// update the view direction of the camera
	camera.direction = vec3(cameraMat * vec4(camera.direction, 0.0f));

	// recalculate the local direction vectors
	camera.target = camera.position + camera.direction;
	camera.right = normalize(cross(camera.direction, up_global));
	camera.up = normalize(cross(camera.right, camera.direction));

	if (curr_x < 20 || curr_x > width - 20) {
		curr_x = center_x;
		// glutWarpPointer(curr_x, curr_y);
	}

	if (curr_y < 20 || curr_y > height - 20) {
		curr_y = center_y;
		// glutWarpPointer(curr_x, curr_y);
	}

	last_x = curr_x;
	last_y = curr_y;
}

int main()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Plane rotation", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to create a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	int gl_version = gladLoadGL(glfwGetProcAddress);
	if (gl_version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}


	glfwWindowHint(GLFW_SAMPLES, 4);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	init();


	do
	{
		updateScene();
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	while (!glfwWindowShouldClose(window));

	glfwTerminate();
	return 0;
}
