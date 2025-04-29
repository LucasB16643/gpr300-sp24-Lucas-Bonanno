#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include "ew/procGen.h"
#include <array>
#include <iostream>
using namespace std;


ew::CameraController cameraController;

ew::Transform monkeyTransform;

ew::Camera camera;


void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void resetCamera(ew::Camera* camera, ew::CameraController* controller);
float inverseLerp(float prevTime, float nextTime, float currentTime) {return (currentTime - prevTime) / (nextTime - prevTime); };
glm::vec3 lerp(glm::vec3 prev, glm::vec3 next, float time) { return float(1.0 - time) * prev + (time * next); };


//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

unsigned int fbo;
unsigned int dummyVAO;
unsigned int textureColorbuffer;
unsigned int depthBuffer;

float lightPosX = -2.0f;
float lightPosY = 4.0f;
float lightPosZ = -1.0f;
float minBias = .0005;
float maxBias = .005;

int numPosFrames = 2;
int numRotFrames = 2;
int numScaleFrames = 2;

bool newPosFrame = false;
bool newRotFrame = false;
bool newscaleFrame = false;




class Vec3Key {
public:
	float time;
	glm::vec3 value;

};



class AnimationClip {
public:
	float duration;
	Vec3Key* positionKey[2];
	Vec3Key* rotationKey[2];
	Vec3Key* scaleKey[2];

	
};
AnimationClip newClip;


class Animator {
public:
	AnimationClip* clip;
	bool isPlaying;
	float playbackSpeed;
	bool isLooping;
	float playbackTime;
};


struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

void animate(float cTime, AnimationClip keyFrames) {
	int numFrames = size(keyFrames.positionKey);
	Vec3Key* nextPos;
	Vec3Key* prevPos;
	Vec3Key* nextRot;
	Vec3Key* prevRot;
	Vec3Key* nextScale;
	Vec3Key* prevScale;

	

	for (int i = 0; i < numFrames; i++) {
		if (keyFrames.positionKey[0]->time >= cTime) {
			monkeyTransform.position = keyFrames.positionKey[0]->value;
			break;
		}
		else if (keyFrames.positionKey[i]->time > cTime) {
			 nextPos = keyFrames.positionKey[i];

			 prevPos = keyFrames.positionKey[i - 1];

			 float t = inverseLerp(prevPos->time, nextPos->time, cTime);

			 glm::vec3 v = lerp(prevPos->value, nextPos->value, t);
			 monkeyTransform.position = v;

			 break;

		}
		else if (keyFrames.positionKey[numFrames - 1]->time < cTime) {
			monkeyTransform.position = keyFrames.positionKey[numFrames - 1]->value;
			break;
		}

	}

	

	for (int i = 0; i < numFrames; i++) {
		if (keyFrames.rotationKey[0]->time >= cTime) {
			monkeyTransform.rotation = keyFrames.rotationKey[0]->value;
			break;
		}
		else if (keyFrames.rotationKey[i]->time > cTime) {
			nextRot = keyFrames.rotationKey[i];

			prevRot = keyFrames.rotationKey[i - 1];

			float t = inverseLerp(prevRot->time, nextRot->time, cTime);

			 glm::vec3 v = lerp(prevRot->value, nextRot->value, t);
			monkeyTransform.rotation = v;

			break;

		}
		else if (keyFrames.rotationKey[numFrames - 1]->time < cTime) {
			monkeyTransform.rotation = keyFrames.rotationKey[numFrames - 1]->value;
			break;
		}

	}

	

	for (int i = 0; i < numFrames; i++) {
		if (keyFrames.scaleKey[0]->time >= cTime) {
			monkeyTransform.scale = keyFrames.scaleKey[0]->value;
			break;
		}
		else if (keyFrames.scaleKey[i]->time > cTime) {
			nextScale = keyFrames.scaleKey[i];

			prevScale = keyFrames.scaleKey[i - 1];

			float t = inverseLerp(prevScale->time, nextScale->time, cTime);

			glm::vec3 v = lerp(prevScale->value, nextScale->value, t);
			monkeyTransform.scale = v;

			break;

		}
		else if (keyFrames.scaleKey[numFrames - 1]->time < cTime) {
			monkeyTransform.scale = keyFrames.scaleKey[numFrames - 1]->value;
			break;
		}

	}
	
	


};

/*void newKeyFrame(AnimationClip clip) {

	if (newPosFrame = true) {
		Vec3Key* newPos[size(clip.positionKey) + 1];
		for (int i = 0; i < size(clip.positionKey); i++) {
			newPos[i] = clip.positionKey[i];
		}
		newPos[size(clip.positionKey) - 1]->time = clip.duration;
		newPos[size(clip.positionKey) - 1]->value = glm::vec3(0, 0, 0);
		clip.positionKey = newPos;
	}



}*/

unsigned int depthMap;
float frametime = 0;

int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader depthShader = ew::Shader("assets/depthShader.vert", "assets/depthShader.frag");
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");
	GLuint brickTexture = ew::loadTexture("assets/new_color.jpg");
	ew::Model mountModel = ew::Model("assets/MountainTerrain.obj");
	
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing

	


	Vec3Key* firstPos = new Vec3Key;
	firstPos->time = 0;
	firstPos->value = glm::vec3(0, 0, 0);

	Vec3Key* secondPos = new Vec3Key;
	secondPos->time = 2;
	secondPos->value = glm::vec3(0, 2, 0);

	Vec3Key* firstRot = new Vec3Key;
	firstRot->time = 0;
	firstRot->value = glm::vec3(0, 0, 0);

	Vec3Key* secondRot = new Vec3Key;
	secondRot->time = 2;
	secondRot->value = glm::vec3(0, 1, 0);


	Vec3Key* firstScale = new Vec3Key;
	firstScale->time = 0;
	firstScale->value = glm::vec3(1, 1, 1);

	Vec3Key* secondScale = new Vec3Key;
	secondScale->time = 2;
	secondScale->value = glm::vec3(1, 1, 1);
	//AnimationClip newClip;

	newClip.positionKey[0] =  firstPos;
	newClip.positionKey[1] = secondPos;
	newClip.rotationKey[0] = firstRot;
	newClip.rotationKey[1] = secondRot;
	newClip.scaleKey[0] = firstScale;
	newClip.scaleKey[1] = secondScale;
	newClip.duration = 10;

	Animator animator;
	animator.clip = &newClip;
	animator.playbackTime = 0;
	animator.isPlaying = true;
	animator.isLooping = true;
	animator.playbackSpeed = 1;

	 numPosFrames = size(newClip.positionKey);
	 numRotFrames = size(newClip.rotationKey);
	 numScaleFrames = size(newClip.scaleKey);

	



	float planeVertices[] = {
		// positions            // normals         // texcoords
		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
	};
	


	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	
	glGenTextures(1, &depthMap);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);


	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);



	/*glCreateVertexArrays(1, &dummyVAO);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, screenWidth, screenHeight);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	*/

	GLuint depthVAO;
	glCreateVertexArrays(1, &depthVAO);
	
	//plane
	float planeWidth = 10.0f;
	float planeHeight = 10.0f;
	int planeSubdivisions = 4;

	//Create Shapes
	ew::Mesh planeMesh(ew::createPlane(planeWidth, planeHeight, planeSubdivisions));

	ew::Transform planeTransform;
	planeTransform.position.x = 1.5f;
	planeTransform.position.y = -2.0f;
	planeTransform.position.z = 0.0f;

	

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		animator.playbackTime += std::fmod((time - prevFrameTime), newClip.duration);
		 if (animator.playbackTime >= newClip.duration && animator.isLooping == false) {
			 animator.playbackTime = newClip.duration;
		}
		else if (animator.playbackTime >= newClip.duration) {
			animator.playbackTime = 0;
		}
		
		prevFrameTime = time;
		float frametime;

		//RENDER
		
		glm::vec3 lightPos(lightPosX, lightPosY, lightPosZ);
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));
		monkeyTransform.rotation;
		if (animator.isPlaying) {
			animate(animator.playbackTime, newClip);
		}

		cameraController.move(window, &camera, deltaTime);

		glCullFace(GL_FRONT);

		float near_plane = 1.0f, far_plane = 10.0f;
		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(lightPos,
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		//glDisable(GL_CULL_FACE);
		depthShader.use();
		depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_BACK);
		depthShader.setMat4("model", monkeyTransform.modelMatrix());
		monkeyModel.draw();


		depthShader.setMat4("model", planeTransform.modelMatrix());
		mountModel.draw();
		
		glCullFace(GL_BACK);



		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glViewport(0, 0, screenWidth, screenHeight);
		glEnable(GL_DEPTH_TEST);//depth testing
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//Bind brick texture to texture unit 0 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, brickTexture);
		//Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
	
		//glBindTextureUnit(1, brickTexture);
		//glBindTextureUnit(0, depthMap);


		shader.use();
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);
		shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		shader.setFloat("minBias", minBias);
		shader.setFloat("maxBias", maxBias);
		shader.setInt("_ShadowMap", 0);
		shader.setInt("_MainTex", 1);
		shader.setVec3("_LightDirection", lightPos);
		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setVec3("_EyePos", camera.position);

		monkeyModel.draw();

		shader.setMat4("_Model", planeTransform.modelMatrix());
		mountModel.draw();

		/*glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.6f, 0.09f, 0.92f, 1.0f);
		// Bind the texture we just rendered to for reading
		glBindTextureUnit(0, textureColorbuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);*/
		
		//glBindVertexArray(depthVAO);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		


		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}
	if (ImGui::CollapsingHeader("Shadows"))
	{
		if (ImGui::Button("Reset Light"))
		{
			lightPosX = -2.0f;
			lightPosY = 4.0f;
			lightPosZ = -1.0f;
		}
	
		ImGui::SliderFloat("Light X", &lightPosX, 0.0f, 20.0f);
		ImGui::SliderFloat("Light Y", &lightPosY, 0.0f, 20.0f);
		ImGui::SliderFloat("Light Z", &lightPosZ, -10.0f, 10.0f);
		ImGui::SliderFloat("Min Bias", &minBias, 0.0005f, .005f);
		ImGui::SliderFloat("Max Bias", &maxBias, .0005f, .005f);

	}
	if (ImGui::CollapsingHeader("Position Keys")) {
		for (int i = 0; i < numPosFrames; i++) {
			ImGui::PushID(i);
			ImGui::SliderFloat3("value", &newClip.positionKey[i]->value.x, 0, 10);
			ImGui::SliderFloat("time", &newClip.positionKey[i]->time, 0, 10);
			ImGui::PopID();

		}
		
	}

	if (ImGui::CollapsingHeader("Rotation Keys")) {
		for (int i = 0; i < numRotFrames; i++) {
			ImGui::PushID(i + numPosFrames);
			ImGui::SliderFloat3("value", &newClip.rotationKey[i]->value.x, 0, 10);
			ImGui::SliderFloat("time", &newClip.rotationKey[i]->time, 0, 10);
			ImGui::PopID();

		}

	}

	if (ImGui::CollapsingHeader("Scale Keys")) {
		for (int i = 0; i < numScaleFrames; i++) {
			ImGui::PushID(i + numPosFrames + numRotFrames);
			ImGui::SliderFloat3("value", &newClip.scaleKey[i]->value.x, 0, 10);
			ImGui::SliderFloat("time", &newClip.scaleKey[i]->time, 0, 10);
			ImGui::PopID();

		}

	}


	ImGui::End();
	ImGui::Begin("Shadow Map");
	//Using a Child allow to fill all the space of the window.
	ImGui::BeginChild("Shadow Map");
	//Stretch image to be window size
	ImVec2 windowSize = ImGui::GetWindowSize();
	//Invert 0-1 V to flip vertically for ImGui display
	//shadowMap is the texture2D handle
	ImGui::Image((ImTextureID)depthMap, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();


	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}


