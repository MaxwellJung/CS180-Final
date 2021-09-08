/*
 * Program 4 example with diffuse and spline camera PRESS 'g'
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn (spline D. McGirr)
 */

#include <iostream>
#include <chrono>
#include <set>
#include <utility>
#include <vector>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "Spline.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program for textures
	std::shared_ptr<Program> texProg;

	// Our shader program for skybox
	std::shared_ptr<Program> cubeProg;

	// Our geometry
	shared_ptr<Shape> cube;
	vector<shared_ptr<Shape>> dummy;
	vec3 dummyMin, dummyMax;
	shared_ptr<Shape> leafblower;

	vector<vector<shared_ptr<Shape>>> tree;
	vector<vector<vec3>> treeloc;

	shared_ptr<Shape> fruit;
	shared_ptr<Shape> apple;
	shared_ptr<Shape> banana;
	shared_ptr<Shape> melon;
	shared_ptr<Shape> orange;
	shared_ptr<Shape> watermelon;

	// hitboxes
	pair<vec3, vec3> groundHitbox;
	vector<pair<vec3, vec3>> dummyHitbox;
	pair<vec3, vec3> leafblowerHitbox;
	vector<vector<pair<vec3, vec3>>> trunkHitbox;

	// global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	// ground VAO
	GLuint GroundVertexArrayID;

	// textures
	shared_ptr<Texture> groundtex;
	shared_ptr<Texture> plastictex;
	shared_ptr<Texture> grasstex;

	shared_ptr<Texture> barkDif;
	shared_ptr<Texture> barkNor;
	shared_ptr<Texture> barkSpec;
	shared_ptr<Texture> leafDif;
	shared_ptr<Texture> leafNor;
	shared_ptr<Texture> leafSpec;

	shared_ptr<Texture> dummytex;

	shared_ptr<Texture> fruittex;
	shared_ptr<Texture> appletex;
	shared_ptr<Texture> bananatex;
	shared_ptr<Texture> melontex;
	shared_ptr<Texture> orangetex;
	shared_ptr<Texture> watermelontex;

	// skybox cubemap images
	vector<std::string> faces {
		"px.jpg",
		"nx.jpg",
		"py.jpg",
		"ny.jpg",
		"pz.jpg",
		"nz.jpg"
	};

	unsigned int cubeMapTexture;

	// dummy animation variables
	vector<Spline> dummySpline;
	vector<glm::vec3> dummyAngle;

	// scene data
	float skyboxSize = 400; // width of skybox
	int treeCount = 50;
	float treeHeight = 25;
	float dummyHeight = 6; // equivalent to 6ft human
	vec3 dummyPos = vec3(0,0,0);
	vec3 lightPos = vec3(100,500,120);
	float lightTrans = 0;
	bool leafblowerOn = true;
	float gravity = -32; // gravity in feet per second per second
	float dropThreshold = 2; // time the leaf blower has to blow for the fruit to drop
	vector<vector<float>> fruitTime; // stores the number of seconds the fruits were exposed to windblower
	vector<vector<vec3>> fruitPos; // stores the initial position of the fruits;
	int fruitVariety = 5; // number of fruit species
	int fruitCount = 20; // number of fruits per tree

	//camera
	bool FPV = true; // (F)irst (P)erson (V)iew
	double g_phi, g_theta = 180;
	double dphi, dtheta;
	vec3 g_eye = vec3(0, 0, 0); // arbitrary value that later gets overwritten
	vec3 g_camera = vec3(0, 5, 0); // location of static camera
	vec3 g_lookAt = vec3(1, 0, 0); // arbitrary value that later gets overwritten
	vec3 up = vec3(0, 1, 0);
	vec3 gaze = g_lookAt-g_eye;

	mat4 View;

	vec2 turnDirection;
	vec3 moveDirection;
	float speed = 6.1; // average walking speed in feet per second

	//movement keys
	set<int> movementKeys = {GLFW_KEY_SPACE, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D};
	float forward, backward, left, right, top, bottom;
	set<int> turnKeys = {GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN};

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans += 50;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans -= 50;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS){
			FPV = !FPV;
		}

		if (movementKeys.find(key) != movementKeys.end())
		{
			if (action == GLFW_PRESS)
			{
				switch (key)
				{
					case GLFW_KEY_SPACE: top = 1; //move Up
					break;
					case GLFW_KEY_LEFT_CONTROL: bottom = 1; //move Down
					break;
					case GLFW_KEY_A: left = 1; //move Left
					break;
					case GLFW_KEY_D: right = 1; //move Right
					break;
					case GLFW_KEY_W: forward = 1; //move Forward
					break;
					case GLFW_KEY_S: backward = 1; //move Backward
					break;
				}
			}
			else if (action == GLFW_RELEASE)
			{
				switch (key)
				{
					case GLFW_KEY_SPACE: top = 0; //move Up
					break;
					case GLFW_KEY_LEFT_CONTROL: bottom = 0; //move Down
					break;
					case GLFW_KEY_A: left = 0; //move Left
					break;
					case GLFW_KEY_D: right = 0; //move Right
					break;
					case GLFW_KEY_W: forward = 0; //move Forward
					break;
					case GLFW_KEY_S: backward = 0; //move Backward
					break;
				}
			}
		}

		if (turnKeys.find(key) != turnKeys.end())
		{
			if (action == GLFW_PRESS)
			{
				switch (key)
				{
					case GLFW_KEY_LEFT: turnDirection += vec2(-1,0); //Yaw Left
					break;
					case GLFW_KEY_RIGHT: turnDirection += vec2(1,0); //Yaw Right
					break;
					case GLFW_KEY_UP: turnDirection += vec2(0,1); //Pitch Up
					break;
					case GLFW_KEY_DOWN: turnDirection += vec2(0,-1); //Pitch Down
					break;
				}
			}
			else if (action == GLFW_RELEASE)
			{
				switch (key)
				{
					case GLFW_KEY_LEFT: turnDirection -= vec2(-1,0); //Yaw Left
					break;
					case GLFW_KEY_RIGHT: turnDirection -= vec2(1,0); //Yaw Right
					break;
					case GLFW_KEY_UP: turnDirection -= vec2(0,1); //Pitch Up
					break;
					case GLFW_KEY_DOWN: turnDirection -= vec2(0,-1); //Pitch Down
					break;
				}
			}
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;

			 leafblowerOn = !leafblowerOn;
		}
	}


	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
   		cout << "xDel + yDel " << deltaX << " " << deltaY << endl;

   		g_theta += deltaX;
   		g_phi += deltaY;
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		// alpha blending
		// glEnable(GL_BLEND);
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag.glsl");
		texProg->init();
		texProg->addUniform("time");
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("shading");
		texProg->addUniform("useNorTex");
		texProg->addUniform("tex");
		texProg->addUniform("norTex");
		texProg->addUniform("specTex");
		texProg->addUniform("ads");
		texProg->addUniform("shine");
		texProg->addUniform("lightPos");
		texProg->addUniform("leafBlowerPos");
		texProg->addUniform("windDir");
		texProg->addUniform("windDir");
		texProg->addUniform("blowable");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		initTextures(resourceDirectory);

		// Initialize the GLSL program that we will use for skybox mapping
		cubeProg = make_shared<Program>();
		cubeProg->setVerbose(true);
		cubeProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
		cubeProg->init();
		cubeProg->addUniform("P");
		cubeProg->addUniform("V");
		cubeProg->addUniform("M");
		cubeProg->addUniform("skybox");
		cubeProg->addAttribute("vertPos");
		cubeProg->addAttribute("vertNor");

		initKeyframe();

		tree = vector<vector<shared_ptr<Shape>>>(3);
		treeloc = vector<vector<vec3>>(tree.size());
		trunkHitbox = vector<vector<pair<vec3, vec3>>>(tree.size(), vector<pair<vec3, vec3>>(treeCount));

		fruitTime = vector<vector<float>>(fruitVariety);
		fruitPos = vector<vector<vec3>>(fruitVariety);

		for (int v = 0; v < tree.size(); v++)
		{
			for (int j = 0; j < treeCount; j++)
			{
				float x, z;
				do
				{
					x = rng(-0.5, 0.5);
					z = rng(-0.5, 0.5);
				}
				while (abs(skyboxSize*x) < 1 && abs(skyboxSize*z) < 1);

				treeloc[v].push_back(skyboxSize*vec3(x,0,z));

				for (int k = 0; k < fruitCount; k++)
				{
					int i = rng(0, fruitVariety);
					vec3 offset = vec3(rng(-0.4,0.4),
									   rng(0.5,1),
									   rng(-0.4,0.4));
					fruitPos[i].push_back(treeloc[v][j]+treeHeight*offset);
					fruitTime[i].push_back(0);
				}
			}
		}
	}

	void initKeyframe()
	{
		// initialize dummy walking keyframes
		dummySpline = vector<Spline>(29);
		dummyAngle = vector<vec3>(29);

		vector<vector<glm::vec3>> dummyKeyframe(29);
		dummyAngle[0] = vec3(0,90,0);
		dummyAngle[3] = vec3(-20,60,0);
		dummyAngle[5] = vec3(0,30,0);
		dummyKeyframe[9] = {vec3(40,0,-75), vec3(30,0,-75), vec3(15,0,-75), vec3(-10,0,-75), vec3(-25,0,-75), vec3(-30,0,-75), vec3(-15,0,-75), vec3(0,0,-75), vec3(15,0,-75), vec3(30,0,-75)};
		dummyKeyframe[11] = {vec3(0,-15,0), vec3(0,-20,0), vec3(0,-25,0), vec3(0,-45,0), vec3(0,-70,0), vec3(0,-72,0), vec3(0,-65,0), vec3(0,-20,0), vec3(0,-10,0), vec3(0,-13,0)};
		dummyKeyframe[23] = {vec3(-36,0,0), vec3(-30,0,0), vec3(-12,0,0), vec3(10,0,0), vec3(22,0,0), vec3(28,0,0), vec3(18,0,0), vec3(-20,0,0), vec3(-25,0,0), vec3(-30,0,0)};
		dummyKeyframe[25] = {vec3(38,0,0), vec3(35,0,0), vec3(30,0,0), vec3(18,0,0), vec3(14,0,0), vec3(10,0,0), vec3(40,0,0), vec3(60,0,0), vec3(38,0,0), vec3(30,0,0)};
		dummyKeyframe[27] = {vec3(-10,0,0), vec3(-15,0,0), vec3(-18,0,0), vec3(-10,0,0), vec3(-8,0,0), vec3(0,0,0), vec3(17,0,0), vec3(9,0,0), vec3(5,0,0), vec3(0,0,0)};

		dummyKeyframe[17] = {vec3(28,0,0), vec3(18,0,0), vec3(-20,0,0), vec3(-25,0,0), vec3(-30,0,0), vec3(-36,0,0), vec3(-30,0,0), vec3(-12,0,0), vec3(10,0,0), vec3(22,0,0)};
		dummyKeyframe[19] = {vec3(10,0,0), vec3(40,0,0), vec3(60,0,0), vec3(38,0,0), vec3(30,0,0), vec3(38,0,0), vec3(35,0,0), vec3(30,0,0), vec3(18,0,0), vec3(14,0,0)};
		dummyKeyframe[21] = {vec3(0,0,0), vec3(17,0,0), vec3(9,0,0), vec3(5,0,0), vec3(0,0,0), vec3(-10,0,0), vec3(-15,0,0), vec3(-18,0,0), vec3(-10,0,0), vec3(-8,0,0)};

		float duration = 1.6;
		for (int i = 0; i < 29; i++)
		{
			dummySpline[i] = Spline(dummyKeyframe[i], duration);
		}
	}

	void initGeom(const std::string& resourceDirectory)
	{
		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information.We'll ignore them for this assignment.

 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		// load in the mesh and make the shape(s)
 		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/cube.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			cube = make_shared<Shape>();
			cube->createShape(TOshapes[0]);
			cube->measure();
			cube->init();
		}

		// Initialize dummy meshes
 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/dummy.obj").c_str());
		
		if (!rc) {
			cerr << errStr << endl;
		} else {
			for (int i = 0; i < 29; i++)
			{
				dummy.push_back(make_shared<Shape>());
				dummy[i]->createShape(TOshapes[i]);
				dummy[i]->measure();
				dummy[i]->init();

				//read out information stored in the shape about its size - something like this...
				//then do something with that information.....
				dummyMin.x = dummy[i]->min.x < dummyMin.x ? dummy[i]->min.x : dummyMin.x;
				dummyMin.y = dummy[i]->min.y < dummyMin.y ? dummy[i]->min.y : dummyMin.y;
				dummyMin.z = dummy[i]->min.z < dummyMin.z ? dummy[i]->min.z : dummyMin.z;

				dummyMax.x = dummy[i]->max.x > dummyMax.x ? dummy[i]->max.x : dummyMax.x;
				dummyMax.y = dummy[i]->max.y > dummyMax.y ? dummy[i]->max.y : dummyMax.y;
				dummyMax.z = dummy[i]->max.z > dummyMax.z ? dummy[i]->max.z : dummyMax.z;
			}
		}

		// Initialize leafblower mesh
 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/leafblower.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			leafblower = make_shared<Shape>();
			leafblower->createShape(TOshapes[0]);
			leafblower->measure();
			leafblower->init();
		}

		// Initialize tree mesh (variation 1~3)
		for (int v = 1; v <= 3; v++)
		{
	 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/tree_mango_var0" + to_string(v) + ".obj").c_str());
			if (!rc) {
				cerr << errStr << endl;
			} else {
				for (int i = 0; i < TOshapes.size(); i++)
				{
					tree[v-1].push_back(make_shared<Shape>());
					tree[v-1][i]->createShape(TOshapes[i]);
					tree[v-1][i]->measure();
					tree[v-1][i]->init();
				}
			}
		}

		// Initialize apple mesh
 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/apple.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			apple = make_shared<Shape>();
			apple->createShape(TOshapes[0]);
			apple->measure();
			apple->init();
		}

		// Initialize banana mesh
 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/banana.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			banana = make_shared<Shape>();
			banana->createShape(TOshapes[0]);
			banana->measure();
			banana->init();
		}

		// Initialize melon mesh
 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/melon.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			melon = make_shared<Shape>();
			melon->createShape(TOshapes[0]);
			melon->measure();
			melon->init();
		}

		// Initialize orange mesh
 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/orange.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			orange = make_shared<Shape>();
			orange->createShape(TOshapes[0]);
			orange->measure();
			orange->init();
		}

		// Initialize watermelon mesh
 		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/watermelon.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			watermelon = make_shared<Shape>();
			watermelon->createShape(TOshapes[0]);
			watermelon->measure();
			watermelon->init();
		}

		//code to load in the ground plane (CPU defined data passed to GPU)
		initGround();

		//load in skybox
		cubeMapTexture = createSky(resourceDirectory + "/skybox/grass/", faces);

		// initialize hitboxes
		dummyHitbox = vector<pair<vec3, vec3>>(29);
	}

	void initTextures(const std::string& resourceDirectory)
	{
		//read in a load the texture
		groundtex = make_shared<Texture>();
		groundtex->setFilename(resourceDirectory + "/skybox/grass/ny.jpg");
		groundtex->init();
		groundtex->setUnit(0);
		groundtex->setWrapModes(GL_REPEAT, GL_REPEAT);

		dummytex = make_shared<Texture>();
		dummytex->setFilename(resourceDirectory + "/wood.jpg");
		dummytex->init();
		dummytex->setUnit(0);
		dummytex->setWrapModes(GL_REPEAT, GL_REPEAT);

		plastictex = make_shared<Texture>();
		plastictex->setFilename(resourceDirectory + "/plastic.jpg");
		plastictex->init();
		plastictex->setUnit(0);
		plastictex->setWrapModes(GL_REPEAT, GL_REPEAT);

		barkDif = make_shared<Texture>();
		barkDif->setFilename(resourceDirectory + "/tree_mangoBark_DIF.tga");
		barkDif->init();
		barkDif->setUnit(0);
		barkDif->setWrapModes(GL_REPEAT, GL_REPEAT);

		barkNor = make_shared<Texture>();
		barkNor->setFilename(resourceDirectory + "/tree_mangoBark_NMM.tga");
		barkNor->init();
		barkNor->setUnit(1);
		barkNor->setWrapModes(GL_REPEAT, GL_REPEAT);

		barkSpec = make_shared<Texture>();
		barkSpec->setFilename(resourceDirectory + "/tree_mangoBark_SPC.tga");
		barkSpec->init();
		barkSpec->setUnit(2);
		barkSpec->setWrapModes(GL_REPEAT, GL_REPEAT);

		leafDif = make_shared<Texture>();
		leafDif->setFilename(resourceDirectory + "/tree_mangoLeaves_DIF.tga");
		leafDif->init();
		leafDif->setUnit(0);
		leafDif->setWrapModes(GL_REPEAT, GL_REPEAT);

		leafNor = make_shared<Texture>();
		leafNor->setFilename(resourceDirectory + "/tree_mangoLeaves_NMM.tga");
		leafNor->init();
		leafNor->setUnit(1);
		leafNor->setWrapModes(GL_REPEAT, GL_REPEAT);

		leafSpec = make_shared<Texture>();
		leafSpec->setFilename(resourceDirectory + "/tree_mangoLeaves_SPC.tga");
		leafSpec->init();
		leafSpec->setUnit(2);
		leafSpec->setWrapModes(GL_REPEAT, GL_REPEAT);

		appletex = make_shared<Texture>();
		appletex->setFilename(resourceDirectory + "/apple.png");
		appletex->init();
		appletex->setUnit(0);
		appletex->setWrapModes(GL_REPEAT, GL_REPEAT);

		bananatex = make_shared<Texture>();
		bananatex->setFilename(resourceDirectory + "/banana.png");
		bananatex->init();
		bananatex->setUnit(0);
		bananatex->setWrapModes(GL_REPEAT, GL_REPEAT);

		melontex = make_shared<Texture>();
		melontex->setFilename(resourceDirectory + "/melon.png");
		melontex->init();
		melontex->setUnit(0);
		melontex->setWrapModes(GL_REPEAT, GL_REPEAT);

		orangetex = make_shared<Texture>();
		orangetex->setFilename(resourceDirectory + "/orange.png");
		orangetex->init();
		orangetex->setUnit(0);
		orangetex->setWrapModes(GL_REPEAT, GL_REPEAT);

		watermelontex = make_shared<Texture>();
		watermelontex->setFilename(resourceDirectory + "/watermelon.png");
		watermelontex->init();
		watermelontex->setUnit(0);
		watermelontex->setWrapModes(GL_REPEAT, GL_REPEAT);
	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = skyboxSize;
		float g_groundY = 0;

  		// A x-z plane at y = g_groundY of dimension g_groundSize^2
		float GrndPos[] = {
			-g_groundSize/2, g_groundY, -g_groundSize/2,
			-g_groundSize/2, g_groundY,  g_groundSize/2,
			g_groundSize/2, g_groundY,  g_groundSize/2,
			g_groundSize/2, g_groundY, -g_groundSize/2
		};

		groundHitbox.first = vec3(-g_groundSize/2, g_groundY, -g_groundSize/2);
		groundHitbox.second = vec3(g_groundSize/2, g_groundY,  g_groundSize/2);

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, skyboxSize/5,
      		skyboxSize/5, skyboxSize/5,
      		skyboxSize/5, 0 };

      	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		//generate the ground VAO
      	glGenVertexArrays(1, &GroundVertexArrayID);
      	glBindVertexArray(GroundVertexArrayID);

      	g_GiboLen = 6;
      	glGenBuffers(1, &GrndBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndNorBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndTexBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

      	glGenBuffers(1, &GIndxBuffObj);
     	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    }

    //code to draw the ground plane
    void drawGround(shared_ptr<Program> curS)
    {
     	// curS->bind();
     	glBindVertexArray(GroundVertexArrayID);
     	groundtex->bind(curS->getUniform("tex"));
		
		//draw the ground plane 
  		SetModel(vec3(0, 0, 0), 0, 0, 1, curS);
  		glEnableVertexAttribArray(0);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(1);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
  		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(2);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
  		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   		// draw!
  		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

  		glDisableVertexAttribArray(0);
  		glDisableVertexAttribArray(1);
  		glDisableVertexAttribArray(2);
  		// curS->unbind();
    }

    void drawDummy(shared_ptr<MatrixStack> Model, shared_ptr<Program> shader){
		int i;
		Model->pushMatrix();
			i = 0; // hip
			// scale down to unit size
			Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
			Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
			Model->rotate(radians(dummyAngle[i].y-g_theta), vec3(0, 1, 0));
			Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
			Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));

			// waist and torso
			{
				for (i = 1; i <= 2; i++)
				{
					Model->pushMatrix();
						Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i]->min.y+dummy[i-1]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
						Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
						Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
						Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
						Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i]->min.y+dummy[i-1]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
				}

					// right arm
					{
						for (i = 3; i <= 8; i++)
						{
							Model->pushMatrix();
								Model->translate(vec3((dummy[i-1]->min.x+dummy[i]->max.x)/2, (dummy[i]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
								if (i == 3)
									Model->rotate(radians(dummyAngle[i].x-g_phi), vec3(1, 0, 0));
								else
									Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
								Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
								Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
								Model->translate(vec3(-(dummy[i-1]->min.x+dummy[i]->max.x)/2, -(dummy[i]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
						}
								i = 8; // leaf blower
								Model->pushMatrix();
									Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
									Model->scale(vec3(60/(maximum(leafblower->max.x-leafblower->min.x, leafblower->max.y-leafblower->min.y, leafblower->max.z-leafblower->min.z))));
									Model->translate(vec3(-32, -35, 0));
									Model->rotate(radians(90.f), vec3(1, 0, 0));
									Model->rotate(radians(180.f), vec3(0, 1, 0));
									Model->rotate(radians(0.f), vec3(0, 0, 1));
									Model->translate(vec3(-(leafblower->min.x+leafblower->max.x)/2, -(leafblower->min.y+leafblower->max.y)/2, -(leafblower->min.z+leafblower->max.z)/2));

									setModel(shader, Model);
									plastictex->bind(shader->getUniform("tex"));
									// Draw leafblower
									leafblowerHitbox.first = Model->topMatrix()*vec4(leafblower->min,1);
									leafblowerHitbox.second = Model->topMatrix()*vec4(leafblower->max,1);
									leafblower->draw(shader);
									dummytex->bind(shader->getUniform("tex"));
								Model->popMatrix();
						for (i = 8; i >= 3; i--)
						{
								setModel(shader, Model);
								dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
								dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
								dummy[i]->draw(shader);
							Model->popMatrix();
						}
					}

					i = 9; // left arm
					{
						Model->pushMatrix();
							
							Model->translate(vec3((dummy[i]->min.x+dummy[i-7]->max.x)/2, (dummy[i]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
							Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
							Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
							Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
							Model->translate(vec3(-(dummy[i]->min.x+dummy[i-7]->max.x)/2, -(dummy[i]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
							for (i = 10; i <= 14; i++)
							{
								Model->pushMatrix();
									Model->translate(vec3((dummy[i]->min.x+dummy[i-1]->max.x)/2, (dummy[i]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
									Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
									Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
									Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
									Model->translate(vec3(-(dummy[i]->min.x+dummy[i-1]->max.x)/2, -(dummy[i]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
							}
							for (i = 14; i >= 10; i--)
							{
									setModel(shader, Model);
									dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
									dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
									dummy[i]->draw(shader);
								Model->popMatrix();
							}
							setModel(shader, Model);
							i = 9;
							dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
							dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
							dummy[i]->draw(shader);
						Model->popMatrix();
					}

					// head and neck
					{
						Model->pushMatrix();
							i = 15; // neck
							Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i]->min.y+dummy[i-13]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
							Model->rotate(radians(dummyAngle[i].x-g_phi), vec3(1, 0, 0));
							Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
							Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
							Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i]->min.y+dummy[i-13]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
							Model->pushMatrix();
								i = 16; // head
								Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i]->min.y+dummy[i-1]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
								Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
								Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
								Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
								Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i]->min.y+dummy[i-1]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
								setModel(shader, Model);
								g_eye = Model->topMatrix()*vec4((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i]->min.y+dummy[i]->max.y)/2, dummy[i]->max.z, 1);
								dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
								dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
								dummy[i]->draw(shader);
							Model->popMatrix();
							setModel(shader, Model);
							i = 15;
							dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
							dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
							dummy[i]->draw(shader);
						Model->popMatrix();
					}

				for (i = 2; i >= 1; i--)
				{
						setModel(shader, Model);
						dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
						dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
						dummy[i]->draw(shader);
					Model->popMatrix();
				}
			}

			i = 17; // right leg
			{
				Model->pushMatrix();
					Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i-17]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
					Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
					Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
					Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
					Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i-17]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
					for (i = 18; i <= 22; i++)
					{
						Model->pushMatrix();
							Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i-1]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
							Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
							Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
							Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
							Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i-1]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
					}
					for (i = 22; i >= 18; i--)
					{
							setModel(shader, Model);
							dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
							dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
							dummy[i]->draw(shader);
						Model->popMatrix();
					}
					setModel(shader, Model);
					i = 17;
					dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
					dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
					dummy[i]->draw(shader);
				Model->popMatrix();
			}

			i = 23; // left leg
			{
				Model->pushMatrix();
					Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i-23]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
					Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
					Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
					Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
					Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i-23]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
					for (i = 24; i <= 28; i++)
					{
						Model->pushMatrix();
							Model->translate(vec3((dummy[i]->min.x+dummy[i]->max.x)/2, (dummy[i-1]->min.y+dummy[i]->max.y)/2, (dummy[i]->min.z+dummy[i]->max.z)/2));
							Model->rotate(radians(dummyAngle[i].x), vec3(1, 0, 0));
							Model->rotate(radians(dummyAngle[i].y), vec3(0, 1, 0));
							Model->rotate(radians(dummyAngle[i].z), vec3(0, 0, 1));
							Model->translate(vec3(-(dummy[i]->min.x+dummy[i]->max.x)/2, -(dummy[i-1]->min.y+dummy[i]->max.y)/2, -(dummy[i]->min.z+dummy[i]->max.z)/2));
					}
					for (i = 28; i >= 24; i--)
					{
							setModel(shader, Model);
							dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
							dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
							dummy[i]->draw(shader);
						Model->popMatrix();
					}
					setModel(shader, Model);
					i = 23;
					dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
					dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
					dummy[i]->draw(shader);
				Model->popMatrix();
			}

			setModel(shader, Model);
			i = 0;
			dummyHitbox[i].first = Model->topMatrix()*vec4(dummy[i]->min,1);
			dummyHitbox[i].second = Model->topMatrix()*vec4(dummy[i]->max,1);
			dummy[i]->draw(shader);
		Model->popMatrix();
	}

    unsigned int createSky(string dir, vector<string> faces)
    {
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false);
		for(GLuint i = 0; i < faces.size(); i++) {
			unsigned char *data =
				stbi_load((dir+faces[i]).c_str(), &width, &height, &nrChannels, 0);
				if (data) {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				} else {
					cout << "failed to load: " << (dir+faces[i]).c_str() << endl;
				}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		cout << " creating cube map any errors : " << glGetError() << endl;
		return textureID;
	}

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS)
  	{
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M)
	{
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

   	void updateTurn()
   	{
   		if (length(turnDirection) > 0)
		{
			g_theta += normalize(turnDirection).x;
			g_phi += normalize(turnDirection).y;
		}

		if (g_phi >= 89)
   			g_phi = 89;
 		else if (g_phi <= -89)
 			g_phi = -89;
   	}

   	void updateMovement(float dt)
   	{
   		moveDirection = (top-bottom)*normalize(up) +
   						(right-left)*normalize(cross(gaze, up)) +
   						(forward-backward)*normalize(vec3(gaze.x, 0, gaze.z));

   		if (length(moveDirection) > 0.1)
   		{
   			for (int i = 0; i < 29; i++)
   			{
   				dummyHitbox[i].first += speed*normalize(moveDirection)*dt;
   				dummyHitbox[i].second += speed*normalize(moveDirection)*dt;
   			}
   			leafblowerHitbox.first += speed*normalize(moveDirection)*dt;
   			leafblowerHitbox.second += speed*normalize(moveDirection)*dt;

   			if (checkCollision() == false)
   			{
   				g_eye += speed*normalize(moveDirection)*dt;
   				dummyPos += speed*normalize(moveDirection)*dt;

   				walk(dt);
   			}
   		}
   	}

   	void updateView()
   	{
   		gaze.x = cos(radians(g_phi))*cos(radians(g_theta));
   		gaze.y = sin(radians(g_phi));
   		gaze.z = cos(radians(g_phi))*cos(radians(90.f)-radians(g_theta));

   		if (FPV)
	   		View = glm::lookAt(g_eye, g_eye + gaze, up);
   		else
   			View = glm::lookAt(g_camera, g_lookAt, up);
   	}

   	bool checkCollision()
   	{
   		bool collision = false;
   		for (int i = 0; i < 29; i++)
   		{
   			collision = collision || checkCollision(dummyHitbox[i], groundHitbox);
   			for (int v = 0; v < tree.size(); v++)
   			{
   				for (int j = 0; j < treeloc[v].size(); j++)
   				{
   					collision = collision || checkCollision(dummyHitbox[i], trunkHitbox[v][j]);
   				}
   			}
   		}

   		for (int v = 0; v < tree.size(); v++)
   		{
   			for (int j = 0; j < treeloc[v].size(); j++)
   			{
   				collision = collision || checkCollision(leafblowerHitbox, trunkHitbox[v][j]);
   			}
   		}

   		collision = collision || checkCollision(leafblowerHitbox, groundHitbox);

   		return collision;
   	}

   	// checkCollision() helper function
   	// checks collision between two hitboxes
   	bool checkCollision(pair<vec3, vec3> box1, pair<vec3, vec3> box2)
   	{
   		vec3 min1 = box1.first;
   		vec3 max1 = box1.second;
   		vec3 min2 = box2.first;
   		vec3 max2 = box2.second;

   		return isOverlapping(min1.x, max1.x, min2.x, max2.x) &&
   			   isOverlapping(min1.y, max1.y, min2.y, max2.y) &&
   			   isOverlapping(min1.z, max1.z, min2.z, max2.z);
   	}

   	// collision helper function
   	bool isOverlapping(float min1, float max1, float min2, float max2)
   	{
   		return (max1 > min2) && (max2 > min1);
   	}

   	void walk(float dt)
   	{
   		if (dot(vec3(gaze.x, 0, gaze.z), moveDirection) < 0)
   			dt = -1*dt;

		for (int i = 0; i < 29; i++)
		{
			if (dummySpline[i].update(dt))
				dummyAngle[i] = dummySpline[i].getPosition();
		}
   	}

   	void updateFruit(float dt)
   	{
   		for (int i = 0; i < fruitVariety; i++)
   		{
   			for (int j = 0; j < fruitTime[i].size(); j++)
   			{
   				if ((leafblowerOn && 
   					length(fruitPos[i][j]-g_eye) < 50 &&
   					dot(normalize(fruitPos[i][j]-g_eye), gaze) > cos(radians(20.f))) ||
   					fruitTime[i][j] >= dropThreshold)
   					fruitTime[i][j] += dt;
   			}
   		}

   		dropFruit(dt);
   	}

   	void dropFruit(float dt)
   	{
   		for (int i = 0; i < fruitVariety; i++)
   		{
   			for (int j = 0; j < fruitPos[i].size(); j++)
   			{
   				if (fruitTime[i][j] >= dropThreshold)
   				{
   					float t = fruitTime[i][j] - dropThreshold;
   					float dr = gravity*t*dt;

   					if (fruitPos[i][j].y + dr > 0)
   						fruitPos[i][j].y += dr;
   				}
   			}
   		}
   	}

   	float maximum(float a, float b, float c)
	{
		float max = (a > b) ? a : b;
		return ((max > c) ? max : c);
	}

	float rng(float LO, float HI)
   	{
   		return LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));
   	}

	void render(float frametime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Aspect ratio
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 2*skyboxSize);

		auto Model = make_shared<MatrixStack>();

		updateTurn();
		updateMovement(frametime);
		updateView();

		// Draw Skybox
		cubeProg->bind();
		glDepthFunc(GL_LEQUAL);
		glUniformMatrix4fv(cubeProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(cubeProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		Model->pushMatrix();
			Model->scale(vec3(skyboxSize));
			setModel(cubeProg, Model);
			//bind the cube map texture
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
			cube->draw(cubeProg);
		Model->popMatrix();
		glDepthFunc(GL_LESS);
		cubeProg->unbind();
		
		texProg->bind();
		glUniform1f(texProg->getUniform("time"), glfwGetTime());
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniform3f(texProg->getUniform("lightPos"), lightPos.x, lightPos.y, lightPos.z+lightTrans);
		glUniform3f(texProg->getUniform("leafBlowerPos"), g_eye.x, g_eye.y, g_eye.z);
		glUniform3f(texProg->getUniform("windDir"), gaze.x, gaze.y, gaze.z);

		// Draw ground
		glUniform1i(texProg->getUniform("blowable"), 0);
		glUniform1i(texProg->getUniform("shading"), 1);
		glUniform1f(texProg->getUniform("shine"), 1);
		glUniform3f(texProg->getUniform("ads"), 1, 1, 0);
		drawGround(texProg);

		// Draw dummy with leaf blower
		Model->pushMatrix();
			Model->translate(dummyPos);
			// Model->rotate(M_PI/2, vec3(0, 1, 0));
			Model->scale(vec3(dummyHeight));
			Model->scale(vec3(1/(dummyMax.y-dummyMin.y)));
			Model->translate(vec3(0, (dummyMax.y-dummyMin.y)/2, 0));
			Model->translate(vec3(-(dummyMin.x+dummyMax.x)/2, -(dummyMin.y+dummyMax.y)/2, -(dummyMin.z+dummyMax.z)/2));
			g_lookAt = Model->topMatrix()*vec4((dummyMin.x+dummyMax.x)/2, (dummyMin.y+dummyMax.y)/2, (dummyMin.z+dummyMax.z)/2, 1);
			glUniform1i(texProg->getUniform("blowable"), 0);
			glUniform1i(texProg->getUniform("shading"), 1);
			glUniform1f(texProg->getUniform("shine"), 100);
			glUniform3f(texProg->getUniform("ads"), 1, 1, 1);
			dummytex->bind(texProg->getUniform("tex"));
			drawDummy(Model, texProg);
		Model->popMatrix();

		for (int v = 0; v < tree.size(); v++)
		{
			for (int j = 0; j < treeloc[v].size(); j++)
			{
				// Draw tree
				Model->pushMatrix();
					Model->translate(treeloc[v][j]);
					Model->scale(vec3(treeHeight));
					Model->scale(vec3(1/(tree[v][0]->max.y-tree[v][0]->min.y)));
					Model->translate(vec3(0, (tree[v][0]->max.y-tree[v][0]->min.y)/2, 0));
					Model->translate(vec3(-(tree[v][0]->min.x+tree[v][0]->max.x)/2, -(tree[v][0]->min.y+tree[v][0]->max.y)/2, -(tree[v][0]->min.z+tree[v][0]->max.z)/2));
					setModel(texProg, Model);

					// tree trunk
					glUniform1i(texProg->getUniform("blowable"), 0);
					glUniform1i(texProg->getUniform("shading"), 1);
					glUniform1f(texProg->getUniform("shine"), 1);
					glUniform3f(texProg->getUniform("ads"), 0, 0, 0);
					barkDif->bind(texProg->getUniform("tex"));
					barkNor->bind(texProg->getUniform("norTex"));
					barkSpec->bind(texProg->getUniform("specTex"));
					trunkHitbox[v][j].first = Model->topMatrix()*vec4((tree[v][0]->min.x+tree[v][0]->max.x)/2.f-0.15f*(tree[v][0]->max.x-tree[v][0]->min.x),
																tree[v][0]->min.y, 
																(tree[v][0]->min.z+tree[v][0]->max.z)/2.f-0.15f*(tree[v][0]->max.z-tree[v][0]->min.z),
																1);
					trunkHitbox[v][j].second = Model->topMatrix()*vec4((tree[v][0]->min.x+tree[v][0]->max.x)/2.f+0.15f*(tree[v][0]->max.x-tree[v][0]->min.x),
																 tree[v][0]->max.y, 
																 (tree[v][0]->min.z+tree[v][0]->max.z)/2.f+0.15f*(tree[v][0]->max.z-tree[v][0]->min.z),
																 1);
					tree[v][0]->draw(texProg);

					// tree leaves
					glUniform1i(texProg->getUniform("blowable"), leafblowerOn);
					leafDif->bind(texProg->getUniform("tex"));
					leafNor->bind(texProg->getUniform("norTex"));
					leafSpec->bind(texProg->getUniform("specTex"));
					tree[v][1]->draw(texProg);
				Model->popMatrix();
			}
		}

		updateFruit(frametime);

		// draw fruits
		for (int i = 0; i < fruitVariety; i++)
		{
			float fruitSize, fruitShine;
			switch (i)
			{
				case 0:
					fruit = apple;
					fruittex = appletex;
					fruitSize = 0.3;
					fruitShine = 80;
					break;
				case 1:
					fruit = banana;
					fruittex = bananatex;
					fruitSize = 0.5;
					fruitShine = 1;
					break;
				case 2:
					fruit = melon;
					fruittex = melontex;
					fruitSize = 0.8;
					fruitShine = 1;
					break;
				case 3:
					fruit = orange;
					fruittex = orangetex;
					fruitSize = 0.4;
					fruitShine = 5;
					break;
				case 4:
					fruit = watermelon;
					fruittex = watermelontex;
					fruitSize = 1.2;
					fruitShine = 20;
					break;
				default:
					fruit = apple;
					fruittex = appletex;
					fruitSize = 0.3;
					fruitShine = 80;
					break;
			}
			for (int j = 0; j < fruitPos[i].size(); j++)
			{
				Model->pushMatrix();
					Model->translate(fruitPos[i][j]);
					Model->scale(vec3(fruitSize));
					Model->scale(vec3(1/(maximum(fruit->max.x-fruit->min.x, fruit->max.y-fruit->min.y, fruit->max.z-fruit->min.z))));
					Model->translate(vec3(-(fruit->min.x+fruit->max.x)/2, -(fruit->min.y+fruit->max.y)/2, -(fruit->min.z+fruit->max.z)/2));
					setModel(texProg, Model);
					fruittex->bind(texProg->getUniform("tex"));
					glUniform1i(texProg->getUniform("blowable"), 0);
					glUniform1i(texProg->getUniform("shading"), 1);
					glUniform1f(texProg->getUniform("shine"), fruitShine);
					glUniform3f(texProg->getUniform("ads"), 1, 1, 1);
					fruit->draw(texProg);
				Model->popMatrix();
			}
		}
		texProg->unbind();

		// Pop matrix stacks.
		Projection->popMatrix();
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(1280, 720);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
