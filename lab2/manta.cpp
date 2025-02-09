// Some parts of this file use code from Lab 04

// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <stack>
#include <cstdlib>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Project includes
#include "maths_funcs.h"

using namespace std;

#pragma region SimpleTypes

typedef mat4(*InstantiateFn)();

// define the default camera properties, with 'direction' moving along the local y-axis
typedef struct {
	vec3 position = vec3(0.0f, 20.0f, 120.0f);
	vec3 target = vec3(0.0f, 0.0f, 0.0f);
	vec3 right = vec3(1.0f, 0.0f, 0.0f);
	vec3 up = vec3(0.0f, 1.0f, 0.0f);
	vec3 direction = vec3(0.0f, 0.0f, -1.0f);
	GLfloat pitch = 0.0f;
	GLfloat yaw = 0.0f;
	GLfloat translateSpeed = 1.0f;
	GLfloat rotateSpeed = 0.5f;
	GLfloat maxRotation = 60.0f;
} Camera;

typedef struct {
	std::string name;
	vec4 baseColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	GLfloat alpha = 0.0f;
	vec4 specularColor;
	GLfloat specularExp;
	GLint shader;
	GLint randomIndex = -1;
	GLfloat emissionStrength = 0.0f;
	bool isEmitting;
	unsigned int diffuseMap;
	unsigned int alphaMap;
} MaterialData;

typedef struct {
	int vao;
	unsigned int materialIndex;
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} MeshData;

typedef struct {
	const char* path;
	std::vector<InstantiateFn> instantiateFns;
}  ModelParams;

typedef struct {
	std::vector<InstantiateFn> instantiateFns;
	std::vector<MeshData> submeshes;
	std::vector<MaterialData> materials;
} ModelData;

#pragma endregion SimpleTypes

GLuint baseShader;
GLuint emissionShader;
std::vector<GLuint*> shaders = { &baseShader, &emissionShader };

// window dimensions
int width = 1200;
int height = 800;
int center_x = int(width / 2);
int center_y = int(height / 2);

// a stack that allows children models to access model matrices from their parent models
static std::stack<mat4> hierarchyStack;

// animation parameters
GLfloat rotate_y = 0.0f;
GLfloat fan_rotate_y = 0.0f;
GLfloat sub_translate_y = 0.0f;
GLfloat sub_rotate_z = 0.0f;

GLfloat signalAmp = 1.0f;
GLfloat windowState = 1.0f;

bool isBlackout = false;
int randomRange = 8;

vec3 up_global = vec3(0.0f, 1.0f, 0.0f);

std::vector<ModelData> models;
Camera camera;

// functions for placing model instances and pushing their model
// matrices on the stack for their children models to use

static mat4 renderBuildings_left() {
	mat4 model = identity_mat4();
	model = translate(model, vec3(-29.6f, 0.0f, 13.5f));
	hierarchyStack.push(model);
	return model;
}

static mat4 renderBuildings_right() {
	mat4 model = identity_mat4();
	model = rotate_y_deg(model, 130.0f);
	model = translate(model, vec3(29.6f, 0.0f, 13.5f));
	hierarchyStack.push(model);
	return model;
}

static mat4 renderTube_right() {
	mat4 model = identity_mat4();
	return model;
}

static mat4 renderTube_left() {
	mat4 model = identity_mat4();
	model = rotate_y_deg(model, -130.0f);
	return model;
}

static mat4 renderTurbine_left() {
	mat4 model = identity_mat4();
	vec3 globalLoc = vec3(-16.11f, 1.91f, 4.12f);
	model = rotate_y_deg(model, 24.4f);
	model = translate(model, globalLoc);
	hierarchyStack.push(model);
	return model;
}

static mat4 renderTurbine_right() {
	mat4 model = identity_mat4();
	vec3 globalLoc = vec3(16.11f, 1.91f, 4.12f);
	model = rotate_y_deg(model, -24.4f);
	model = translate(model, globalLoc);
	hierarchyStack.push(model);
	return model;
}

static mat4 renderSub_back() {
	mat4 model = identity_mat4();
	vec3 globalLoc = vec3(-20.0f, 0.0f, 20.0f);
	//model = rotate_z_deg(model, -sub_rotate_z);
	model = translate(model, globalLoc.operator*(-1));
	model = translate(model, vec3(0.0f, sub_translate_y, 0.0f));
	model = rotate_x_deg(model, 70.0f);
	model = rotate_y_deg(model, 120.0f);
	model = translate(model, globalLoc);
	hierarchyStack.push(model);
	return model;
}

static mat4 renderSub_front() {
	mat4 model = identity_mat4();
	vec3 globalLoc = vec3(50.0f, 0.0f, 40.0f);
	//model = rotate_z_deg(model, sub_rotate_z);
	model = translate(model, globalLoc.operator*(-1));
	model = translate(model, vec3(0.0f, sub_translate_y, 0.0f));
	model = rotate_x_deg(model, 90.0f);
	model = rotate_y_deg(model, -30.0f);
	model = translate(model, globalLoc);
	hierarchyStack.push(model);
	return model;
}

static mat4 renderSub_middle() {
	mat4 model = identity_mat4();
	vec3 globalLoc = vec3(-40.0f, 60.0f, 30.0f);
	//model = rotate_z_deg(model, sub_rotate_z);
	model = translate(model, globalLoc.operator*(-1));
	model = translate(model, vec3(0.0f, sub_translate_y, 0.0f));
	model = rotate_x_deg(model, 60.0f);
	model = rotate_y_deg(model, 100.0f);
	model = translate(model, globalLoc);
	hierarchyStack.push(model);
	return model;
}

static mat4 renderFan_right() {
	mat4 model = identity_mat4();
	model = rotate_y_deg(model, fan_rotate_y);
	model = rotate_x_deg(model, 90.0f);
	model = rotate_y_deg(model, -24.87f);
	model = translate(model, vec3(13.61f, 2.27f, 9.64f));
	return model;
}

static mat4 renderFan_left() {
	mat4 model = identity_mat4();
	model = rotate_y_deg(model, fan_rotate_y);
	model = rotate_x_deg(model, 90.0f);
	model = rotate_y_deg(model, 24.87f);
	model = translate(model, vec3(-13.61f, 2.27f, 9.64f));
	return model;
}

static mat4 renderFan_sub() {
	mat4 parentModelX = hierarchyStack.top();
	mat4 model = identity_mat4();
	model = scale(model, vec3(0.6, 1.1, 0.6));
	model = rotate_y_deg(model, fan_rotate_y * 1.8f);
	model = rotate_x_deg(model, 180.0f);
	model = translate(model, vec3(0.0f, -2.3f, 0.0f));
	model = parentModelX * model;
	hierarchyStack.pop();
	return model;
}

static mat4 renderDome_left() {
	mat4 parentModelX = hierarchyStack.top();
	mat4 model = identity_mat4();
	model = parentModelX * model;
	hierarchyStack.pop();
	return model;
}

static mat4 renderDome_right() {
	mat4 parentModelX = hierarchyStack.top();
	mat4 model = identity_mat4();
	model = parentModelX * model;
	hierarchyStack.pop();
	return model;
}

static mat4 renderGlass_right() {
	renderBuildings_right();
	mat4 parentModelX = hierarchyStack.top();
	mat4 model = identity_mat4();
	model = parentModelX * model;
	hierarchyStack.pop();
	return model;
}

static mat4 renderGlass_left() {
	renderBuildings_left();
	mat4 parentModelX = hierarchyStack.top();
	mat4 model = identity_mat4();
	model = parentModelX * model;
	hierarchyStack.pop();
	return model;
}

const std::string texturePath = "textures/";

// define model objects and their instances
static std::vector<ModelParams> modelPaths = {
	{"meshes/plane.obj", {identity_mat4}},
	{"meshes/rocks.obj", {identity_mat4}},
	{"meshes/base_center.obj", {identity_mat4}},
	{"meshes/tube.obj", {renderTube_right, renderTube_left}},
	{"meshes/turbine.obj", {renderTurbine_left, renderTurbine_right}},
	{"meshes/sub.obj", {renderSub_back, renderSub_front, renderSub_middle}},
	{"meshes/fan.obj", {renderFan_sub, renderFan_sub, renderFan_sub, renderFan_right, renderFan_left}},
	{"meshes/buildings.obj", {renderBuildings_right, renderBuildings_left}},
	{"meshes/dome.obj", {renderDome_left, renderDome_right, identity_mat4}},
	{"meshes/glass_ext.obj", {renderGlass_right, renderGlass_left, identity_mat4}},
	{"meshes/glass_int.obj", {renderGlass_right, renderGlass_left, identity_mat4}},
};

#pragma region MESH LOADING

int createTexture(const char* path) {
	int width;
	int height;
	int channelCount;
	unsigned char* pixels = stbi_load(path, &width, &height, &channelCount, 0);
	unsigned int textureHandle;

	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// for JPG textures
	GLint channelDepth = GL_RGB;
	if (channelCount == 4) {
		// for PNG textures with alpha channel
		channelDepth = GL_RGBA;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, channelDepth, width, height, 0, channelDepth, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	stbi_image_free(pixels);
	return textureHandle;
}

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

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
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

		aiString name;
		aiMaterial->Get(AI_MATKEY_NAME, name);
		material.name = name.C_Str();

		// 'w_' = window, 's_' = signal; both of which are emissive materials
		if (material.name[1] == '_') {
			if (material.name[0] == 'w') material.randomIndex = rand() % randomRange;
			material.emissionStrength = 1.0f;
			material.isEmitting = true;
			material.shader = emissionShader;
		}
		else {
			material.shader = baseShader;
		}

		GLfloat sExp;
		aiMaterial->Get(AI_MATKEY_SHININESS, sExp);
		material.specularExp = sExp;

		int diffuseTexCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
		if (diffuseTexCount) {
			aiString imagePath;
			aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &imagePath);
			std::string fullPath = texturePath + std::string(imagePath.C_Str());
			material.diffuseMap = createTexture(fullPath.c_str());
		}
		else {
			aiColor4D bColor;
			aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, bColor);
			material.baseColor = vec4(bColor.r, bColor.g, bColor.b, bColor.a);
		}

		int alphaTexCount = aiMaterial->GetTextureCount(aiTextureType_OPACITY);
		if (alphaTexCount) {
			aiString imagePath;
			aiMaterial->GetTexture(aiTextureType_OPACITY, 0, &imagePath);
			std::string fullPath = texturePath + std::string(imagePath.C_Str());
			material.alphaMap = createTexture(fullPath.c_str());
		}
		else {
			GLfloat alpha;
			aiMaterial->Get(AI_MATKEY_OPACITY, alpha);
			material.alpha = alpha;
		}

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
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

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
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

void CompileShaders()
{
	baseShader = glCreateProgram();
	emissionShader = glCreateProgram();

	if (baseShader + emissionShader == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(baseShader, "shaders/baseVert.glsl", GL_VERTEX_SHADER);
	AddShader(baseShader, "shaders/baseFrag.glsl", GL_FRAGMENT_SHADER);

	AddShader(emissionShader, "shaders/baseVert.glsl", GL_VERTEX_SHADER);
	AddShader(emissionShader, "shaders/emissionFrag.glsl", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(baseShader);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(baseShader, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(baseShader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking base shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	glLinkProgram(emissionShader);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(emissionShader, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(emissionShader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking emission shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	glValidateProgram(baseShader);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(baseShader, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(baseShader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid base shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	glValidateProgram(emissionShader);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(emissionShader, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(emissionShader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid emission shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
}
#pragma endregion SHADER_FUNCTIONS

#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh() {
	for (int i = 0; i < modelPaths.size(); i++) {
		ModelParams currentModel = modelPaths[i];
		ModelData modelData = read_model(currentModel.path);
		std::vector<MeshData>& submeshes = modelData.submeshes;
		modelData.instantiateFns = currentModel.instantiateFns;

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

			GLuint loc1, loc2, loc3;
			loc1 = glGetAttribLocation(baseShader, "vertex_position");
			loc2 = glGetAttribLocation(baseShader, "vertex_normal");
			loc3 = glGetAttribLocation(baseShader, "vertex_texture");

			glEnableVertexAttribArray(loc1);
			glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
			glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

			glEnableVertexAttribArray(loc2);
			glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
			glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

			glEnableVertexAttribArray(loc3);
			glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
			glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);

			// save VAO reference in mesh properties
			currentMesh.vao = vao;
		}
		models.push_back(modelData);
	}
}
#pragma endregion VBO_FUNCTIONS

// write all material properties to the shader
void setMaterial(MaterialData material, mat4 locMat) {
	GLuint currentShader = material.shader;
	glUseProgram(currentShader);

	int modelMatLoc = glGetUniformLocation(currentShader, "model");
	glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, locMat.m);

	GLint baseColorLoc = glGetUniformLocation(currentShader, "baseColor");
	glUniform4fv(baseColorLoc, 1, material.baseColor.v);

	GLint alphaLoc = glGetUniformLocation(currentShader, "alpha");
	glUniform1f(alphaLoc, material.alpha);

	GLint diffuseMapLoc = glGetUniformLocation(currentShader, "diffuseMap");
	glUniform1i(diffuseMapLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, material.diffuseMap);

	GLint alphaMapLoc = glGetUniformLocation(currentShader, "alphaMap");
	glUniform1i(alphaMapLoc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, material.alphaMap);

	if (currentShader == emissionShader) {
		GLint emissionStrengthLoc = glGetUniformLocation(currentShader, "emissionStrength");
		// make signal materials fluctuate in intensity
		if (material.name[0] == 's') {
			glUniform1f(emissionStrengthLoc, material.emissionStrength * signalAmp);
		}
		// randomly switch windows on and off
		else {
			if (windowState == material.randomIndex) material.isEmitting = !material.isEmitting;
			glUniform1f(emissionStrengthLoc, material.emissionStrength * material.isEmitting * !isBlackout);
		}
	}
	else {
		GLint specularColorLoc = glGetUniformLocation(currentShader, "specularColor");
		glUniform4fv(specularColorLoc, 1, material.specularColor.v);

		GLint specExpLoc = glGetUniformLocation(currentShader, "specularExp");
		glUniform1f(specExpLoc, material.specularExp);
	}
}

void display() {
	vec4 backgroundColor = vec4(0.0f, 0.15f, 0.3f, 1.0f);
	vec4 lightPosition = vec4(50.0f, 100.0f, 20.0f, 1.0f);
	float fogSight = 220.0f;

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(backgroundColor.v[0], backgroundColor.v[1], backgroundColor.v[2], backgroundColor.v[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 model = identity_mat4();
	mat4 view = look_at(camera.position, camera.target, camera.up);
	mat4 persp_proj = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);

	vec4 LightPositionView = view * model * lightPosition;
	int LightPosLoc = glGetUniformLocation(baseShader, "lightPosition");
	glUniform3fv(LightPosLoc, 1, LightPositionView.v);

	for (int i = 0; i < shaders.size(); i++) {
		GLuint currentShader = *shaders[i];
		glUseProgram(currentShader);
		int viewMatLoc = glGetUniformLocation(currentShader, "view");
		int projMatLoc = glGetUniformLocation(currentShader, "proj");
		int ambientColorLoc = glGetUniformLocation(currentShader, "ambientColor");
		int fogSightLoc = glGetUniformLocation(currentShader, "fogSight");

		glUniform4fv(ambientColorLoc, 1, backgroundColor.v);
		glUniform1f(fogSightLoc, fogSight);

		glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, persp_proj.m);
		glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, view.m);
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
	glutSwapBuffers();
}


void updateScene() {
	static DWORD last_time = 0;
	static DWORD switchBegin = 0;

	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// update signal intensity
	signalAmp = sin(curr_time * 0.003f) * 0.4f + 0.6f;

	// change window state every 3 seconds
	if (curr_time - switchBegin > 3000) {
		windowState = rand() % randomRange;
		switchBegin = curr_time;
	}

	// animate fan rotation on the submersibles and the turbines
	fan_rotate_y += 40.0f * delta;
	fan_rotate_y = fmodf(fan_rotate_y, 360.0f);

	// animate the submersible translation
	sub_translate_y += 0.7f * delta;

	glutPostRedisplay();
}

void init()
{
	CompileShaders();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	generateObjectBufferMesh();

	// hide and center cursor on load
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(center_x, center_y);
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
	// switch off the lights
	if (key == 'b') {
		isBlackout = !isBlackout;
	}
	// exit program on pressing 'Esc'
	if (key == 27) {
		exit(0);
	}

	// constrain the camera position to a certain volume in the scene
	camera.position.v[0] = min(max(camera.position.v[0], -60.0f), 60.0f);
	camera.position.v[1] = min(max(camera.position.v[1], 4.0f), 40.0f);
	camera.position.v[2] = min(max(camera.position.v[2], -40.0f), 150.0f);

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
	mat4 cameraMat = identity_mat4();
	cameraMat = translate(cameraMat, camera.direction.operator*(-1));

	// rotate according to the deltas
	cameraMat = rotate_y_deg(cameraMat, -delta_x);

	// constrain camera pitch
	if (abs(newPitch) < camera.maxRotation) {
		cameraMat = rotate_x_deg(cameraMat, -delta_y);
		camera.yaw = newYaw;
		camera.pitch = newPitch;
	}

	// reapply initial translation
	cameraMat = translate(cameraMat, camera.direction);

	// update the view direction of the camera
	camera.direction = cameraMat.operator*(vec4((camera.direction), 0.0f));

	// recalculate the local direction vectors
	camera.target = camera.position + camera.direction;
	camera.right = normalise(cross(camera.direction, up_global));
	camera.up = normalise(cross(camera.right, camera.direction));

	if (curr_x < 20 || curr_x > width - 20) {
		curr_x = center_x;
		glutWarpPointer(curr_x, curr_y);
	}

	if (curr_y < 20 || curr_y > height - 20) {
		curr_y = center_y;
		glutWarpPointer(curr_x, curr_y);
	}

	last_x = curr_x;
	last_y = curr_y;
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);

	// anti-aliasing
	glutSetOption(GLUT_MULTISAMPLE, 4);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA | GLUT_MULTISAMPLE);

	glutInitWindowSize(width, height);
	glutCreateWindow("Mantaville");

	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutPassiveMotionFunc(mouseMove);

	GLenum res = glewInit();
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	init();
	glutMainLoop();
	return 0;
}
