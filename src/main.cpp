#include <iostream>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <stack>
#include <cstdlib>

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace std;
using namespace glm;
using vec4 = glm::vec4;
using vec3 = glm::vec3;
using vec2 = glm::vec2;
using mat4 = glm::mat4;

typedef mat4 (*InstantiateFn)();

// define the default camera properties, with 'direction' moving along the local y-axis
struct CameraData {
    vec3 position = vec3(0.0f, 0.0f, 15.0f);
    vec3 target = vec3(0.0f, 0.0f, 0.0f);
    vec3 right = vec3(1.0f, 0.0f, 0.0f);
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 direction = vec3(0.0f, 0.0f, -1.0f);
    GLfloat pitch = 0.0f;
    GLfloat yaw = 0.0f;
    GLfloat translateSpeed = 0.6f;
    GLfloat rotateSpeed = 0.5f;
    GLfloat maxRotation = 60.0f;
};

struct AtmosphereData {
    float rayleighFactor = 1.0f;
    float mieFactor = 1.0f;
    vec3 planetCenter = vec3(0.0f);
    float planetRadius = 0.9f;
    float atmosphereRadius = 1.0f;;
    float atmosphereThickness = 0.1f;
    float gMie = -0.8f;
    vec3 wavelengthPeaks = vec3(0.650f, 0.570f, 0.475f);
    vec3 kRayleigh = vec3(3.8f, 13.5f, 33.1f); // TODO: change
    vec3 kMie = vec3(21.0f); // TODO: change
    int outscatterSteps = 10;
    int inscatterSteps = 30;
    float h0Rayleigh = 0.05f;
    float h0Mie = 0.02f;
};

struct MaterialData {
    std::string name = "material";
    GLuint *shader;
    vec4 baseColor = vec4(0.9f, 0.9f, 0.9f, 1.0f);
    GLfloat alpha = 1.0f;
    vec4 specularColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    GLfloat specularCoeff = 0.2f;
    GLfloat phongExp = 100.0f;
    GLuint diffuseMap;
    GLuint alphaMap;
    GLuint normalMap;
};

struct MeshData {
    int vao;
    unsigned int materialIndex;
    size_t mPointCount = 0;
    std::vector<vec3> mVertices;
    std::vector<vec3> mNormals;
    std::vector<vec2> mTextureCoords;
    std::vector<vec3> mTangents;
    std::vector<vec3> mBitangents;
};

struct ModelParams {
    const char *path;
    const bool *isVisible;
    const std::vector<InstantiateFn> instantiateFns;
    const vector<MaterialData> materials;
};

struct ModelData {
    std::vector<InstantiateFn> instantiateFns;
    std::vector<MeshData> submeshes;
    std::vector<MaterialData> materials;
};

static int maxAnisotropy = 1;

// window dimensions
int width = 1200;
int height = 700;
// int center_x = int(width / 2);
// int center_y = int(height / 2);
float aspectRatio = (float) width / (float) height;
float FoV = 45.0f;

// a stack that allows children models to access model matrices from their parent models
static std::stack<mat4> hierarchyStack;

// global parameters
GLfloat selfRotation = 0.0f;
vec3 up_global = vec3(0.0f, 1.0f, 0.0f);
bool showUI = true;

CameraData thirdPersonCamera = {
    .position = vec3(0.0f, 0.0f, 10.0f),
};
CameraData *camera = &thirdPersonCamera;

const std::string texturePath = "../textures/";
vec4 backgroundColor = vec4(0.2f, 0.2f, 0.2f, 1.0f);
vec4 lightPosition = vec4(0.0f, 0.0f, -10.0f, 1.0f);
vec4 lightColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
float lightIntensity = 10.0f;

GLuint phongShader;
GLuint atmosphereShader;
std::vector<GLuint *> shaders = {&phongShader, &atmosphereShader};

AtmosphereData atmosphereData = {};

MaterialData phongMaterial = {
    .baseColor = lightColor,
    .shader = &phongShader
};

MaterialData atmosphereMaterial = {
    .shader = &atmosphereShader
};

GLfloat specularScale = 1.0f;
GLfloat phongExpScale = 0.05f;

static mat4 renderAtmosphere() {
    mat4 model = mat4(1.0f);
    model = glm::scale(model, vec3(atmosphereData.atmosphereRadius));
    return model;
}

static mat4 renderSun() {
    mat4 model = mat4(1.0f);
    model = glm::scale(model, vec3(0.1f));
    model = glm::translate(model, vec3(lightPosition));
    return model;
}

static mat4 renderSurface() {
    mat4 model = mat4(1.0f);
    return model;
}

// define model objects and their instances
std::vector<ModelData> models;
bool showEarth = true;
bool showAtmosphere = true;
bool showSun = true;
bool showMoon = false;
bool showPlane = false;
bool showHills = false;
bool showCurve = false;
static std::vector<ModelParams> modelPaths = {
    {"../meshes/unit_sphere.obj", &showAtmosphere, {renderAtmosphere}, {atmosphereMaterial}},
    {"../meshes/unit_sphere.obj", &showSun, {renderSun}, {phongMaterial}},
};

#pragma region MESH LOADING

GLuint createTexture(const char *path) {
    int width;
    int height;
    int channelCount;
    unsigned char *pixels = stbi_load(path, &width, &height, &channelCount, 0);
    GLuint textureHandle;

    if (!pixels) {
        std::cout << "could not load texture: " << path << std::endl;
    }

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
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(currentMinConfig));
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(currentMagConfig));
    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);


    stbi_image_free(pixels);
    return textureHandle;
}

void getTangents(MeshData &currentMesh) {
    std::vector<vec3> &vertices = currentMesh.mVertices;
    std::vector<vec2> &texCo = currentMesh.mTextureCoords;
    std::vector<vec3> &normals = currentMesh.mNormals;
    std::vector<vec3> tangents(currentMesh.mPointCount, vec3(0.0f, 0.0f, 0.0f));

    for (int i = 0; i < currentMesh.mPointCount; i += 3) {
        vec3 &v0 = vertices[i + 0];
        vec3 &v1 = vertices[i + 1];
        vec3 &v2 = vertices[i + 2];

        vec2 &uv0 = texCo[i + 0];
        vec2 &uv1 = texCo[i + 1];
        vec2 &uv2 = texCo[i + 2];

        vec3 deltaPos1 = v1 - v0;
        vec3 deltaPos2 = v2 - v0;

        vec2 deltaUV1 = uv1 - uv0;
        vec2 deltaUV2 = uv2 - uv0;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;

        tangents[i + 0] += tangent;
        tangents[i + 1] += tangent;
        tangents[i + 2] += tangent;
    }

    for (int i = 0; i < currentMesh.mPointCount; i++) {
        tangents[i] = normalize((tangents[i] - normals[i] * glm::dot(normals[i], tangents[i])));
    }

    currentMesh.mTangents = tangents;
}

ModelData read_model(const char *file_name) {
    ModelData modelData;
    const aiScene *scene = aiImportFile(
        file_name,
        aiProcess_Triangulate | aiProcess_PreTransformVertices
    );

    if (!scene) {
        fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
        return modelData;
    }

    printf("reading model file: %s\n", file_name);
    printf("  %i meshes\n", scene->mNumMeshes);
    printf("  %i textures\n", scene->mNumTextures);

    for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
        aiMesh *mesh = scene->mMeshes[m_i];
        MeshData submesh;
        submesh.mPointCount = mesh->mNumVertices;
        submesh.materialIndex = mesh->mMaterialIndex;

        for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
            if (mesh->HasPositions()) {
                const aiVector3D *vp = &(mesh->mVertices[v_i]);
                submesh.mVertices.emplace_back(vp->x, vp->y, vp->z);
            }
            if (mesh->HasNormals()) {
                const aiVector3D *vn = &(mesh->mNormals[v_i]);
                submesh.mNormals.emplace_back(vn->x, vn->y, vn->z);
            }
            if (mesh->HasTextureCoords(0)) {
                const aiVector3D *vt = &(mesh->mTextureCoords[0][v_i]);
                submesh.mTextureCoords.emplace_back(vt->x, vt->y);
            }
            if (mesh->HasTangentsAndBitangents()) {
                const aiVector3D *vta = &(mesh->mTangents[v_i]);
                submesh.mTangents.emplace_back(vta->x, vta->y, vta->z);
                const aiVector3D *vbi = &(mesh->mBitangents[v_i]);
                submesh.mTangents.emplace_back(vbi->x, vbi->y, vbi->z);
            }
        }
        if (!submesh.mTangents.size()) {
            printf("  calculating tangents and bitangents for: %s\n", file_name);
            getTangents(submesh);
        }
        modelData.submeshes.push_back(submesh);
    }

    // load materials
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        const aiMaterial *aiMaterial = scene->mMaterials[i];
        MaterialData material;

        aiString name;
        aiMaterial->Get(AI_MATKEY_NAME, name);
        material.name = name.C_Str();

        material.shader = &phongShader;

        GLfloat sExp;
        aiMaterial->Get(AI_MATKEY_SHININESS, sExp);
        material.specularCoeff = sExp;

        int diffuseTexCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
        if (diffuseTexCount) {
            aiString imagePath;
            aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &imagePath);
            std::string fullPath = texturePath + std::string(imagePath.C_Str());
            material.diffuseMap = createTexture(fullPath.c_str());
        } else {
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
        } else {
            GLfloat alpha;
            aiMaterial->Get(AI_MATKEY_OPACITY, alpha);
            material.alpha = alpha;
        }

        int normalTexCount = aiMaterial->GetTextureCount(aiTextureType_HEIGHT);
        if (normalTexCount) {
            aiString imagePath;
            aiMaterial->GetTexture(aiTextureType_HEIGHT, 0, &imagePath);
            std::string fullPath = texturePath + std::string(imagePath.C_Str());
            material.normalMap = createTexture(fullPath.c_str());
        }

        aiColor4D sColor;
        aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, sColor);
        material.specularColor = vec4(sColor.r, sColor.g, sColor.b, sColor.a);

        modelData.materials.emplace_back(material);
    }

    aiReleaseImport(scene);
    return modelData;
}

#pragma endregion MESH LOADING

#pragma region SHADER_FUNCTIONS
char *readShaderSource(const char *shaderFile) {
    FILE *fp = fopen(shaderFile, "rb");

    if (fp == NULL) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char *buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}

static void AddShader(GLuint ShaderProgram, const char *pShaderText, GLenum ShaderType) {
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        std::cerr << "Error creating shader..." << std::endl;
    }
    const char *pShaderSource = readShaderSource(pShaderText);

    glShaderSource(ShaderObj, 1, (const GLchar **) &pShaderSource, NULL);
    glCompileShader(ShaderObj);
    GLint success;

    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024] = {'\0'};
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        std::cerr << "Error compiling "
                << (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
                << " shader program: " << InfoLog << std::endl;
    }

    glAttachShader(ShaderProgram, ShaderObj);
}

void CompileShaders() {
    phongShader = glCreateProgram();

    if (phongShader == 0) {
        std::cerr << "Error creating shader program..." << std::endl;
    }

    AddShader(phongShader, "../shaders/base.vert", GL_VERTEX_SHADER);
    AddShader(phongShader, "../shaders/base.frag", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = {'\0'};

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

    atmosphereShader = glCreateProgram();

    if (atmosphereShader == 0) {
        std::cerr << "Error creating shader program..." << std::endl;
    }

    AddShader(atmosphereShader, "../shaders/atmosphere.vert", GL_VERTEX_SHADER);
    AddShader(atmosphereShader, "../shaders/atmosphere.frag", GL_FRAGMENT_SHADER);

    GLuint VAO2;
    glGenVertexArrays(1, &VAO2);
    glBindVertexArray(VAO2);

    glLinkProgram(atmosphereShader);
    glGetProgramiv(atmosphereShader, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(atmosphereShader, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Error linking atmosphere shader program: " << ErrorLog << std::endl;
    }

    glValidateProgram(atmosphereShader);
    glGetProgramiv(atmosphereShader, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(atmosphereShader, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Invalid atmosphere shader program: " << ErrorLog << std::endl;
    }

    glBindVertexArray(0);
}
#pragma endregion SHADER_FUNCTIONS

#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh() {
    for (int i = 0; i < modelPaths.size(); i++) {
        ModelParams currentModel = modelPaths[i];
        if (!(*currentModel.isVisible))
            continue;
        ModelData modelData = read_model(currentModel.path);
        std::vector<MeshData> &submeshes = modelData.submeshes;
        modelData.instantiateFns = currentModel.instantiateFns;

        // overwrite imported materials
        modelData.materials = currentModel.materials;

        // process all meshes in each loaded model
        for (int j = 0; j < submeshes.size(); j++) {
            MeshData &currentMesh = submeshes[j];
            unsigned int vao = (i + j) * 2;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            unsigned int vp_vbo = vao;
            glGenBuffers(1, &vp_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
            glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec3), &currentMesh.mVertices[0],
                         GL_STATIC_DRAW);

            unsigned int vn_vbo = vao;
            glGenBuffers(1, &vn_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
            glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec3), &currentMesh.mNormals[0],
                         GL_STATIC_DRAW);

            unsigned int vt_vbo = vao;
            glGenBuffers(1, &vt_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
            glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec2), &currentMesh.mTextureCoords[0],
                         GL_STATIC_DRAW);

            unsigned int vta_vbo = vao;
            glGenBuffers(1, &vta_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vta_vbo);
            glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec3), &currentMesh.mTangents[0],
                         GL_STATIC_DRAW);

            unsigned int vbi_vbo = vao;
            glGenBuffers(1, &vbi_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbi_vbo);
            glBufferData(GL_ARRAY_BUFFER, currentMesh.mPointCount * sizeof(vec3), &currentMesh.mBitangents[0],
                         GL_STATIC_DRAW);

            for (int i = 0; i < shaders.size(); i++) {
                GLuint currentShader = *shaders[i];

                GLuint vertPosLoc, vertNorLoc, vertTexLoc, vertTanLoc, vertBitLoc;
                vertPosLoc = glGetAttribLocation(currentShader, "vertexPosition");
                vertNorLoc = glGetAttribLocation(currentShader, "vertexNormal");
                vertTexLoc = glGetAttribLocation(currentShader, "vertexTexture");
                vertTanLoc = glGetAttribLocation(currentShader, "vertexTangent");
                vertBitLoc = glGetAttribLocation(currentShader, "vertexBitangent");

                glEnableVertexAttribArray(vertPosLoc);
                glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
                glVertexAttribPointer(vertPosLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

                glEnableVertexAttribArray(vertNorLoc);
                glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
                glVertexAttribPointer(vertNorLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

                glEnableVertexAttribArray(vertTexLoc);
                glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
                glVertexAttribPointer(vertTexLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);

                glEnableVertexAttribArray(vertTanLoc);
                glBindBuffer(GL_ARRAY_BUFFER, vta_vbo);
                glVertexAttribPointer(vertTanLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

                glEnableVertexAttribArray(vertBitLoc);
                glBindBuffer(GL_ARRAY_BUFFER, vbi_vbo);
                glVertexAttribPointer(vertBitLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            }
            // save VAO reference in mesh properties
            currentMesh.vao = vao;
        }
        models.push_back(modelData);
    }
}
#pragma endregion VBO_FUNCTIONS

// write all material properties to the shader
void setMaterial(MaterialData &material, mat4 locMat) {
    // GLuint currentShader = phongShader;
    // currentShader = atmosphereShader;
    GLuint currentShader = *material.shader;
    glUseProgram(currentShader);

    GLint modelMatLoc = glGetUniformLocation(currentShader, "modelMatrix");
    glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, &locMat[0][0]);

    GLint baseColorLoc = glGetUniformLocation(currentShader, "baseColor");
    glUniform4fv(baseColorLoc, 1, &material.baseColor[0]);

    GLint alphaLoc = glGetUniformLocation(currentShader, "alpha");
    glUniform1f(alphaLoc, material.alpha);

    GLint specularColorLoc = glGetUniformLocation(currentShader, "specularColor");
    glUniform4fv(specularColorLoc, 1, &material.specularColor[0]);

    GLint phongExpLoc = glGetUniformLocation(currentShader, "phongExp");
    glUniform1f(phongExpLoc, material.phongExp * phongExpScale);

    GLint specularCoeffLoc = glGetUniformLocation(currentShader, "specularCoeff");
    glUniform1f(specularCoeffLoc, material.specularCoeff * specularScale);

    GLint diffuseMapLoc = glGetUniformLocation(currentShader, "diffuseMap");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.diffuseMap);
    glUniform1i(diffuseMapLoc, 0);

    GLint alphaMapLoc = glGetUniformLocation(currentShader, "alphaMap");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, material.alphaMap);
    glUniform1i(alphaMapLoc, 1);

    GLint normalMapLoc = glGetUniformLocation(currentShader, "normalMap");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, material.normalMap);
    glUniform1i(normalMapLoc, 2);

    GLint rayleighFactorLoc = glGetUniformLocation(currentShader, "rayleighFactor");
    glUniform1f(rayleighFactorLoc, atmosphereData.rayleighFactor);

    GLint mieFactorLoc = glGetUniformLocation(currentShader, "mieFactor");
    glUniform1f(mieFactorLoc, atmosphereData.mieFactor);

    GLint planetCenterLoc = glGetUniformLocation(currentShader, "planetCenter");
    glUniform3fv(planetCenterLoc, 1, &atmosphereData.planetCenter[0]);

    GLint planetRadiusLoc = glGetUniformLocation(currentShader, "planetRadius");
    glUniform1f(planetRadiusLoc, atmosphereData.planetRadius);

    GLint atmosphereRadiusLoc = glGetUniformLocation(currentShader, "atmosphereRadius");
    glUniform1f(atmosphereRadiusLoc, atmosphereData.atmosphereRadius);

    GLint gMieLoc = glGetUniformLocation(currentShader, "gMie");
    glUniform1f(gMieLoc, atmosphereData.gMie);

    GLint kRayleighLoc = glGetUniformLocation(currentShader, "kRayleigh");
    glUniform3fv(kRayleighLoc, 1, (&atmosphereData.kRayleigh[0]));

    GLint kMieLoc = glGetUniformLocation(currentShader, "kMie");
    glUniform3fv(kMieLoc, 1, &atmosphereData.kMie[0]);

    GLint outscatterStepsLoc = glGetUniformLocation(currentShader, "outscatterSteps");
    glUniform1i(outscatterStepsLoc, atmosphereData.outscatterSteps);

    GLint inscatterStepsLoc = glGetUniformLocation(currentShader, "inscatterSteps");
    glUniform1i(inscatterStepsLoc, atmosphereData.inscatterSteps);

    GLint h0RayleighLoc = glGetUniformLocation(currentShader, "h0Rayleigh");
    glUniform1f(h0RayleighLoc, atmosphereData.atmosphereThickness * atmosphereData.h0Rayleigh);

    GLint h0MieLoc = glGetUniformLocation(currentShader, "h0Mie");
    glUniform1f(h0MieLoc, atmosphereData.atmosphereThickness * atmosphereData.h0Mie);
}

void renderScene() {
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 view = glm::lookAt(camera->position, camera->target, camera->up);
    mat4 proj = glm::perspective(glm::radians(FoV), aspectRatio, 0.1f, 1000.0f);

    vec4 lightPositionView = view * lightPosition;

    for (int i = 0; i < shaders.size(); i++) {
        GLuint currentShader = *shaders[i];
        glUseProgram(currentShader);
        int viewMatLoc = glGetUniformLocation(currentShader, "viewMatrix");
        glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, &view[0][0]);

        int projMatLoc = glGetUniformLocation(currentShader, "projectionMatrix");
        glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, &proj[0][0]);

        int cameraPosLoc = glGetUniformLocation(currentShader, "cameraPosition");
        glUniform3fv(cameraPosLoc, 1, &camera->position[0]);

        int lightPosLoc = glGetUniformLocation(currentShader, "lightPosition");
        glUniform3fv(lightPosLoc, 1, &lightPosition[0]);

        int lightIntensityLoc = glGetUniformLocation(currentShader, "lightIntensity");
        glUniform1f(lightIntensityLoc, lightIntensity);

        int viewLightPosLoc = glGetUniformLocation(currentShader, "viewSpaceLightPosition");
        glUniform3fv(viewLightPosLoc, 1, &lightPositionView[0]);

        int lightColorLoc = glGetUniformLocation(currentShader, "lightColor");
        glUniform3fv(lightColorLoc, 1, &lightColor[0]);
    }

    // instantiate all submeshes of each model
    for (int i = 0; i < models.size(); i++) {
        ModelData &currentModel = models[i];
        int numInstances = currentModel.instantiateFns.size();
        std::vector<MeshData> submeshes = currentModel.submeshes;
        // instantiate and draw meshes with their assigned materials
        for (int k = 0; k < numInstances; k++) {
            mat4 currentLocMat = currentModel.instantiateFns[k]();
            for (int j = 0; j < submeshes.size(); j++) {
                MeshData &currentMesh = submeshes[j];
                MaterialData &currentMaterial = currentModel.materials[currentMesh.materialIndex];
                setMaterial(currentMaterial, currentLocMat);
                glBindVertexArray(currentMesh.vao);
                glDrawArrays(GL_TRIANGLES, 0, currentMesh.mPointCount);
            }
        }
    }
    glBindVertexArray(0);
}

void updateScene() {
    static double last_time = 0;

    double curr_time = glfwGetTime();
    if (last_time == 0)
        last_time = curr_time;
    double delta = (curr_time - last_time);
    last_time = curr_time;

    selfRotation += 30.0f * delta;
    selfRotation = fmodf(selfRotation, 360.0f);
}

void reloadScene() {
    models.clear();
    generateObjectBufferMesh();
}

void drawUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Parameters");
    if (ImGui::Button("Reload scene")) {
        reloadScene();
    }

    ImGui::Text("Background color:");
    ImGui::ColorEdit3("Background Color", &backgroundColor[0]);

    ImGui::Text("Render layers:");
    if (ImGui::Checkbox("Sun", &showSun) ||
        ImGui::Checkbox("Atmosphere", &showAtmosphere)) {
        reloadScene();
    }
    ImGui::Text("Projection:");
    if (ImGui::SliderInt("X", &width, 10, 2000) ||
        ImGui::SliderInt("Y", &height, 10, 2000)) {
        aspectRatio = (float) width / (float) height;
    }
    ImGui::SliderFloat("FoV", &FoV, 1.0f, 360.0f);

    ImGui::Text("Camera pos:");
    ImGui::SliderFloat("Xp", &camera->position.x, -50.0f, 50.0f);
    ImGui::SliderFloat("Yp", &camera->position.y, -50.0f, 50.0f);
    ImGui::SliderFloat("Zp", &camera->position.z, -50.0f, 50.0f);

    ImGui::Text("Camera target:");
    ImGui::SliderFloat("Xt", &camera->target.x, -50.0f, 50.0f);
    ImGui::SliderFloat("Yt", &camera->target.y, -50.0f, 50.0f);
    ImGui::SliderFloat("Zt", &camera->target.z, -50.0f, 50.0f);

    ImGui::Text("Light position:");
    ImGui::SliderFloat("Xl", &lightPosition.x, -100.0f, 100.0f);
    ImGui::SliderFloat("Yl", &lightPosition.y, -100.0f, 100.0f);
    ImGui::SliderFloat("Zl", &lightPosition.z, -100.0f, 100.0f);

    ImGui::Text("Light color:");
    ImGui::ColorEdit3("Light Color", &lightColor[0]);

    ImGui::Text("Light intensity:");
    ImGui::SliderFloat("Intensity", &lightIntensity, 0.0f, 50.0f);

    ImGui::Text("Shader properties:");
    ImGui::SliderFloat("Phong exp scale", &phongExpScale, 0, 5.0f);

    ImGui::Text("Atmosphere properties:");
    ImGui::Text("Atmosphere Thickness: %.3f", atmosphereData.atmosphereThickness);
    ImGui::SliderFloat3("Planet Center", &atmosphereData.planetCenter[0], -100.0f, 100.0f);
    if (ImGui::SliderFloat("Planet Radius", &atmosphereData.planetRadius, 0.1f, atmosphereData.atmosphereRadius) ||
        ImGui::SliderFloat("Atmosphere Radius", &atmosphereData.atmosphereRadius, atmosphereData.planetRadius, 10.0f)) {
        atmosphereData.atmosphereThickness = atmosphereData.atmosphereRadius - atmosphereData.planetRadius;
    };
    ImGui::SliderFloat("Rayleigh Factor", &atmosphereData.rayleighFactor, 0.0f, 1.0f);
    ImGui::SliderFloat("Mie Factor", &atmosphereData.mieFactor, 0.0f, 1.0f);
    ImGui::SliderFloat("g Mie", &atmosphereData.gMie, -0.999f, 0.999f);
    if (ImGui::SliderFloat3("Wavelength Peaks", &atmosphereData.wavelengthPeaks[0], 0.0f, 1.0f)) {
        atmosphereData.kRayleigh[0] = 1.0f/pow(atmosphereData.wavelengthPeaks[0], 4.0f);
        atmosphereData.kRayleigh[1] = 1.0f/pow(atmosphereData.wavelengthPeaks[1], 4.0f);
        atmosphereData.kRayleigh[2] = 1.0f/pow(atmosphereData.wavelengthPeaks[2], 4.0f);
    };
    ImGui::SliderFloat3("K Rayleigh", &atmosphereData.kRayleigh[0], 0.0f, 50.0f);
    ImGui::SliderFloat3("K Mie", &atmosphereData.kMie[0], 0.0f, 50.0f);
    ImGui::SliderInt("In-scattering Steps", &atmosphereData.inscatterSteps, 1, 100);
    ImGui::SliderInt("Out-scattering Steps", &atmosphereData.outscatterSteps, 1, 100);
    ImGui::SliderFloat("H0 Rayleigh", &atmosphereData.h0Rayleigh, 0.01f, 1.0f);
    ImGui::SliderFloat("H0 Mie", &atmosphereData.h0Mie, 0.01f, 1.0f);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        showUI = !showUI;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        reloadScene();

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_W) {
            camera->position += camera->direction * camera->translateSpeed;
        }
        if (key == GLFW_KEY_S) {
            camera->position -= camera->direction * camera->translateSpeed;
        }
        if (key == GLFW_KEY_Q) {
            camera->position += camera->up * camera->translateSpeed;
        }
        if (key == GLFW_KEY_E) {
            camera->position -= camera->up * camera->translateSpeed;
        }
        if (key == GLFW_KEY_D) {
            camera->position += camera->right * camera->translateSpeed;
        }
        if (key == GLFW_KEY_A) {
            camera->position -= camera->right * camera->translateSpeed;
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);

    static GLFWwindow *window;
    static GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    static const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    window = glfwCreateWindow(mode->width, mode->height, "RTR 05 - Atmospheric Scattering", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create a GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, key_callback);

    int gl_version = gladLoadGL(glfwGetProcAddress);
    if (gl_version == 0) {
        std::cerr << "Failed to initialize OpenGL context." << std::endl;
        return -1;
    }

    CompileShaders();
    generateObjectBufferMesh();

    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.Fonts->AddFontFromFileTTF("../fonts/Lato-Bold.ttf", 16.0f);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Upload fonts to GPU (if using OpenGL)
    ImGui_ImplOpenGL3_CreateFontsTexture();

    do {
        updateScene();
        renderScene();
        if (showUI) drawUI();
        glfwSwapBuffers(window);
        glfwPollEvents();
    } while (!glfwWindowShouldClose(window));

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
