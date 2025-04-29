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
#include <vector>
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

struct joint {
	string name;
	 int parentIndex;

};

struct skeleton {
	unsigned int jointCount;
	std::vector<joint*> joints;
};

struct jointPose {
	glm::vec3 rotation; //Euler angle
	glm::vec3 translation; //Translation
	glm::vec3 scale; //Scale
};

struct skeletonPose {
	skeleton* pSkeleton; //Reference to skeleton
	//vector<jointPose> aLocalPose; //Array of joint poses
	vector<ew::Transform> transforms;
	vector<glm::mat4> aGlobalPose; //Global joint poses. This is what is calculated using FK!
};

ew::Transform transform;


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

skeletonPose skellP;



skeletonPose SolveFK(skeletonPose hierarchy)
{
int index = 0;
	for each (joint* joint in hierarchy.pSkeleton->joints) {
		glm::mat4 matrix = hierarchy.transforms[index].modelMatrix();

		if (joint->parentIndex == -1) {

			hierarchy.aGlobalPose[index] = matrix;
		}
		else
		{
			glm::mat4 parent = hierarchy.aGlobalPose[joint->parentIndex];

			hierarchy.aGlobalPose[index] = (parent * matrix);
		}
		index++;
	}
	return hierarchy;
}



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
unsigned int reflectionFrameBuffer;
unsigned int reflectionDepthBuffer;
unsigned int reflectionTexture;
unsigned int refractionFrameBuffer;
unsigned int refractionDepthBuffer;
unsigned int refractionTexture;
float frametime = 0;
ew::Transform secondMonkey;
float moveFact = 0.06;
float waveS = .02;
float shineDamper = 10.0;
float reflectivity = 0.6;

int main() {
	GLFWwindow* window = initWindow("Assignment 2", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader depthShader = ew::Shader("assets/depthShader.vert", "assets/depthShader.frag");
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");
	GLuint brickTexture = ew::loadTexture("assets/new_color.jpg");
	ew::Shader reflectionShader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader refractionShader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader waterShader = ew::Shader("assets/water.vert", "assets/water.frag");
	GLuint dudv = ew::loadTexture("assets/waterDUDV.png");
	GLuint normalMap = ew::loadTexture("assets/matchingNormalMap.png");

	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing
	glEnable(GL_CLIP_DISTANCE0);


	

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

	newClip.positionKey[0] = firstPos;
	newClip.positionKey[1] = secondPos;
	newClip.rotationKey[0] = firstRot;
	newClip.rotationKey[1] = secondRot;
	newClip.scaleKey[0] = firstScale;
	newClip.scaleKey[1] = secondScale;
	newClip.duration = 10;

	Animator animator;
	animator.clip = &newClip;
	animator.playbackTime = 0;
	animator.isPlaying = false;
	animator.isLooping = true;
	animator.playbackSpeed = 1;

	numPosFrames = size(newClip.positionKey);
	numRotFrames = size(newClip.rotationKey);
	numScaleFrames = size(newClip.scaleKey);


	 skeleton* skell = new skeleton;
	 skell->jointCount = 0;
	 
		 

		 joint* torso = new joint;
		 torso->name = "torso";
		 torso->parentIndex = -1;
		 skell->joints.push_back(torso);
		 skell->jointCount++;

		 joint* head = new joint;
		 head->name = "head";
		 head->parentIndex = 0;
		 skell->joints.push_back(head);
		 skell->jointCount++;
		

		 joint* rShoulder = new joint;
		 rShoulder->name = "r_Shoulder";
		 rShoulder->parentIndex = 0;
		 skell->joints.push_back(rShoulder);
		 skell->jointCount++;

		 joint* lShoulder = new joint;
		 lShoulder->name = "l_Shoulder";
		 lShoulder->parentIndex = 0;
		 skell->joints.push_back(lShoulder);
		 skell->jointCount++;

		 joint* rElbow = new joint;
		 rElbow->name = "r_Elbow";
		 rElbow->parentIndex = 2;
		 skell->joints.push_back(rElbow);
		 skell->jointCount++;

		 joint* lElbow = new joint;
		 lElbow->name = "l_Elbow";
		 lElbow->parentIndex = 3;
		 skell->joints.push_back(lElbow);
		 skell->jointCount++;

		 joint* rWrist = new joint;
		 rWrist->name = "r_Wrist";
		 rWrist->parentIndex = 4;
		 skell->joints.push_back(rWrist);
		 skell->jointCount++;

		 joint* lWrist = new joint;
		 lWrist->name = "l_Wrist";
		 lWrist->parentIndex = 5;
		 skell->joints.push_back(lWrist);
		 skell->jointCount++;

		 skellP.pSkeleton = skell;
	 

	 for (int i = 0; i < skell->jointCount; i++) {
		 jointPose pose;
		 ew::Transform tran;
		 glm::mat4 mat;
		 //skellP.aLocalPose.push_back(pose);
		 skellP.transforms.push_back(tran);
		 skellP.aGlobalPose.push_back(mat);
		 
	 }
	 
	 /*skellP.aLocalPose[0].translation = glm::vec3(0, 1, 0);
	 skellP.aLocalPose[1].translation = glm::vec3(0, 1, 0);
	 skellP.aLocalPose[2].translation = glm::vec3(1, 0, 0);
	 skellP.aLocalPose[3].translation = glm::vec3(-1, 0, 0);
	 skellP.aLocalPose[4].translation = glm::vec3(2, 0, 0);
	 skellP.aLocalPose[5].translation = glm::vec3(-2, 0, 0);
	 skellP.aLocalPose[6].translation = glm::vec3(3, 0, 0);
	 skellP.aLocalPose[7].translation = glm::vec3(-3, 0, 0);*/

	 skellP.transforms[0].position = glm::vec3(0, 0, 0);
	 skellP.transforms[1].position = glm::vec3(0, 1, 0);
	 skellP.transforms[2].position = glm::vec3(1, 0, 0);
	 skellP.transforms[3].position = glm::vec3(-1, 0, 0);
	 skellP.transforms[4].position = glm::vec3(2, 0, 0);
	 skellP.transforms[5].position = glm::vec3(-2, 0, 0);
	 skellP.transforms[6].position = glm::vec3(3, 0, 0);
	 skellP.transforms[7].position = glm::vec3(-3, 0, 0);

	 /*skellP.aLocalPose[0].rotation = glm::vec3(0, 0, 0);
	 
	 skellP.aLocalPose[1].rotation = glm::vec3(0, 0, 0);
	 skellP.aLocalPose[2].rotation = glm::vec3(0, 0, 0);
	 skellP.aLocalPose[3].rotation = glm::vec3(0, 0, 0);
	 skellP.aLocalPose[4].rotation = glm::vec3(0, 0, 0);
	 skellP.aLocalPose[5].rotation = glm::vec3(0, 0, 0);
	 skellP.aLocalPose[6].rotation = glm::vec3(0, 0, 0);
	 skellP.aLocalPose[7].rotation = glm::vec3(0, 0, 0);*/

	 skellP.transforms[0].rotation = glm::vec3(0, 0, 0);

	 skellP.transforms[1].rotation = glm::vec3(0, 0, 0);
	 skellP.transforms[2].rotation = glm::vec3(0, 0, 0);
	 skellP.transforms[3].rotation = glm::vec3(0, 0, 0);
	 skellP.transforms[4].rotation = glm::vec3(0, 0, 0);
	 skellP.transforms[5].rotation = glm::vec3(0, 0, 0);
	 skellP.transforms[6].rotation = glm::vec3(0, 0, 0);
	 skellP.transforms[7].rotation = glm::vec3(0, 0, 0);

	 /*skellP.aLocalPose[0].scale = glm::vec3(.5, .5, .5);
	 skellP.aLocalPose[1].scale = glm::vec3(.5, .5, .5);
	 skellP.aLocalPose[2].scale = glm::vec3(.5, .5, .5);
	 skellP.aLocalPose[3].scale = glm::vec3(.5, .5, .5);
	 skellP.aLocalPose[4].scale = glm::vec3(.5, .5, .5);
	 skellP.aLocalPose[5].scale = glm::vec3(.5, .5, .5);
	 skellP.aLocalPose[6].scale = glm::vec3(.5, .5, .5);
	 skellP.aLocalPose[7].scale = glm::vec3(.5, .5, .5);*/

	 skellP.transforms[0].scale = glm::vec3(.5, .5, .5);
	 skellP.transforms[1].scale = glm::vec3(.5, .5, .5);
	 skellP.transforms[2].scale = glm::vec3(.5, .5, .5);
	 skellP.transforms[3].scale = glm::vec3(.5, .5, .5);
	 skellP.transforms[4].scale = glm::vec3(.5, .5, .5);
	 skellP.transforms[5].scale = glm::vec3(.5, .5, .5);
	 skellP.transforms[6].scale = glm::vec3(.5, .5, .5);
	 skellP.transforms[7].scale = glm::vec3(.5, .5, .5);

	 

	 /*skellP.aGlobalPose[0]->translation = glm::vec3(0, 0, 0);
	 skellP.aGlobalPose[1]->translation = glm::vec3(0, 1, 0);
	 skellP.aGlobalPose[2]->translation = glm::vec3(1, 0, 0);
	 skellP.aGlobalPose[3]->translation = glm::vec3(-1, 0, 0);
	 skellP.aGlobalPose[4]->translation = glm::vec3(2, 0, 0);
	 skellP.aGlobalPose[5]->translation = glm::vec3(-2, 0, 0);
	 skellP.aGlobalPose[6]->translation = glm::vec3(3, 0, 0);
	 skellP.aGlobalPose[7]->translation = glm::vec3(-3, 0, 0);
	 */
	 



	 



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

	
	glGenFramebuffers(1, &reflectionFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFrameBuffer);


	glGenTextures(1, &reflectionTexture);
	glBindTexture(GL_TEXTURE_2D, reflectionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture, 0);

	glGenRenderbuffers(1, &reflectionDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth,
		screenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, reflectionDepthBuffer);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &refractionFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, refractionFrameBuffer);


	glGenTextures(1, &refractionTexture);
	glBindTexture(GL_TEXTURE_2D, refractionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionTexture, 0);

	glGenRenderbuffers(1, &refractionDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, refractionDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth,
		screenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, refractionDepthBuffer);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);








	
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

	float floorWidth = 20.0f;
	float floorHeight = 20.0f;
	int floorSubdivisions = 4;

	ew::Mesh floorPlane = ew::createPlane(floorWidth, floorHeight, floorSubdivisions);
	ew::Transform floorTransform;
	floorTransform.position.x = 1.5f;
	floorTransform.position.y = -5.0f;
	floorTransform.position.z = 0.0f;

	
	secondMonkey.position = glm::vec3(0, -4, -1);
	monkeyTransform.position.z = 3;
	monkeyTransform.position.y = 2;
	

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

		float waveSpeed = moveFact * (float)glfwGetTime();
		waveSpeed = std::fmod(waveSpeed, 1);

		//RENDER
		
		glm::vec3 lightPos(lightPosX, lightPosY, lightPosZ);
		//monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));
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

		glDisable(GL_CULL_FACE);
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
		planeMesh.draw();
		
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

		glm::vec3 cameraPosition = camera.position;
		glm::vec3 cameraTarget = camera.target;

		cameraPosition.y = planeTransform.position.y - (camera.position.y - planeTransform.position.y);
		cameraTarget.y = planeTransform.position.y - (cameraTarget.y - planeTransform.position.y);

		glm::mat4 reflectedView = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0, 1, 0));
		glm::mat4 reflectedVP = camera.projectionMatrix() * reflectedView;

		glEnable(GL_CLIP_DISTANCE0);
		glDisable(GL_CULL_FACE); // test only

		glBindFramebuffer(GL_FRAMEBUFFER, reflectionFrameBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		reflectionShader.use();
		reflectionShader.setFloat("_Material.Ka", material.Ka);
		reflectionShader.setFloat("_Material.Kd", material.Kd);
		reflectionShader.setFloat("_Material.Ks", material.Ks);
		reflectionShader.setFloat("_Material.Shininess", material.Shininess);
		reflectionShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		reflectionShader.setFloat("minBias", minBias);
		reflectionShader.setFloat("maxBias", maxBias);
		reflectionShader.setInt("_ShadowMap", 0);
		reflectionShader.setInt("_MainTex", 1);
		reflectionShader.setVec3("_LightDirection", lightPos);
		reflectionShader.setMat4("_Model", monkeyTransform.modelMatrix());
		reflectionShader.setMat4("_ViewProjection", reflectedVP);
		reflectionShader.setVec3("_EyePos", camera.position);
		reflectionShader.setVec4("plane", glm::vec4(0, 1, 0, -planeTransform.position.y));


		glCullFace(GL_FRONT);
		monkeyModel.draw();

		//monkeyTransform.position.y = -4;
		//monkeyTransform.position.z = -1;
		reflectionShader.setMat4("_Model", secondMonkey.modelMatrix());
		monkeyModel.draw();

		reflectionShader.setMat4("_Model", floorTransform.modelMatrix());
		floorPlane.draw();
		glCullFace(GL_BACK);
		//monkeyTransform.position.y = 1;
		//monkeyTransform.position.z = 3;
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, reflectionTexture);
		
		//Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
	
		//glBindTextureUnit(1, brickTexture);
		//glBindTextureUnit(0, depthMap);

		glBindFramebuffer(GL_FRAMEBUFFER, refractionFrameBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		refractionShader.use();
		refractionShader.setFloat("_Material.Ka", material.Ka);
		refractionShader.setFloat("_Material.Kd", material.Kd);
		refractionShader.setFloat("_Material.Ks", material.Ks);
		refractionShader.setFloat("_Material.Shininess", material.Shininess);
		refractionShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		refractionShader.setFloat("minBias", minBias);
		refractionShader.setFloat("maxBias", maxBias);
		refractionShader.setInt("_ShadowMap", 0);
		refractionShader.setInt("_MainTex", 1);
		refractionShader.setVec3("_LightDirection", lightPos);
		refractionShader.setMat4("_Model", monkeyTransform.modelMatrix());
		refractionShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		refractionShader.setVec3("_EyePos", camera.position);
		refractionShader.setVec4("plane", glm::vec4(0, -1, 0, planeTransform.position.y));



		monkeyModel.draw();

		//monkeyTransform.position.y = -4;
		//monkeyTransform.position.z = -1;
		refractionShader.setMat4("_Model", secondMonkey.modelMatrix());
		monkeyModel.draw();

		refractionShader.setMat4("_Model", floorTransform.modelMatrix());
		floorPlane.draw();
		//monkeyTransform.position.y = 1;
		//monkeyTransform.position.z = 3;
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, refractionTexture);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, dudv);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, normalMap);
		/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
		

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glDisable(GL_CULL_FACE);
		glDisable(GL_CLIP_DISTANCE0);
		
		
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
		shader.setVec4("plane", glm::vec4(0, 0, 0, 0));

		monkeyModel.draw();

		//monkeyTransform.position.y = -4;
		//monkeyTransform.position.z = -1;
		shader.setMat4("_Model", secondMonkey.modelMatrix());
		monkeyModel.draw();

		//monkeyTransform.position.y = 1;
		//monkeyTransform.position.z = 3;

		shader.setMat4("_Model", floorTransform.modelMatrix());

		floorPlane.draw();
		
		glDisable(GL_CULL_FACE);
		
		waterShader.use();
		waterShader.setMat4("_ViewProjection", camera.projectionMatrix()* camera.viewMatrix());
		
		waterShader.setMat4("modelMatrix", planeTransform.modelMatrix());
		waterShader.setInt("reflectionTexture", 2);
		waterShader.setInt("refractionTexture", 3);
		waterShader.setInt("dudv", 4);
		waterShader.setFloat("moveFactor", waveSpeed);
		waterShader.setVec3("cameraPosition", camera.position);
		waterShader.setInt("normalMap", 5);
		waterShader.setVec3("lightPosition", lightPos);
		waterShader.setFloat("waveStrength", waveS);
		waterShader.setFloat("shineDamper", shineDamper);
		waterShader.setFloat("reflectivity", reflectivity);

		planeMesh.draw();


		//skellP = SolveFK(skellP);
		
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
	if (ImGui::CollapsingHeader("Transforms"))
	{
		ImGui::SliderFloat3("Monkey Position", &monkeyTransform.position.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Second Position", &secondMonkey.position.x, -10.0f, 10.0f);

	}
	if (ImGui::CollapsingHeader("Water"))
	{
		ImGui::SliderFloat("Move Frator", &moveFact, 0.0f, 1.0f);
		ImGui::SliderFloat("Wave Strength", &waveS, 0.0f, 1.0f);
		ImGui::SliderFloat("Shine Damper", &shineDamper, 0.0f, 50.0f);
		ImGui::SliderFloat("Reflectivity", &reflectivity, 0.0f, 1.0f);

	}
	


	ImGui::End();



	


	ImGui::Begin("reflection Map");
	//Using a Child allow to fill all the space of the window.
	ImGui::BeginChild("reflection Map");
	//Stretch image to be window size
	ImVec2 windowSize = ImGui::GetWindowSize();
	//Invert 0-1 V to flip vertically for ImGui display
	//shadowMap is the texture2D handle
	ImGui::Image((ImTextureID)reflectionTexture, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	
	ImGui::EndChild();
	ImGui::End();

	ImGui::Begin("refraction Map");
	//Using a Child allow to fill all the space of the window.
	ImGui::BeginChild("refraction Map");
	//Stretch image to be window size
	 windowSize = ImGui::GetWindowSize();
	//Invert 0-1 V to flip vertically for ImGui display
	//shadowMap is the texture2D handle
	ImGui::Image((ImTextureID)refractionTexture, windowSize, ImVec2(0, 1), ImVec2(1, 0));

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


