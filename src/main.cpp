#define GLFW_INCLUDE_NONE

// externals
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include <stb_image/stb_image.h>
#include <sstream>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"
#include "Rendering/ShadowMap.h"

// main project dependencies
#include "Core/Primitives.h"
#include "Core/Camera.h"
#include "Core/AssetPaths.h"
#include "Road/RoadGenerator.h"
#include "Road/RoadParams.h"
#include "Rendering/LineRenderer.h"
#include "Lots/LotSubdivision.h"
#include "Lots/LotParams.h"
#include "Lots/LotTypes.h"
#include "Rendering/BuildingRenderer.h"
#include "Export/CityExporter.h"
#include "GUI/Gui.h"
#include <Rendering/Shader.h>
#include "Rendering/Skybox.h"
#include "Generation/CityGenerator.h"
#include "Rendering/CityRenderer.h"
#include "Rendering/RenderContexts.h"

// global Y axis variable for zooming in/out control.
static float gScrollY = 0.0f;

// controls the delayed city-generation overlay workflow
static bool gShowGeneratingOverlay = false;
static bool gGeneratePending = false;

struct InitializationContext
{
	ShadowMap shadowMap;
	Skybox skybox;
	Primitives primitives;
	Camera camera;
};

namespace
{

	BuildingContext sBuilding;
	RoadContext sRoads;
	LotContext sLots;
	TextureContext sTex;
	InitializationContext sInit;

	// draws a simple modal-style overlay while city generation is in progress
	static void DrawGeneratingOverlay()
	{
		if (!gShowGeneratingOverlay)
			return;

		ImGuiViewport *vp = ImGui::GetMainViewport();
		ImVec2 center = vp->GetCenter();

		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(420, 120), ImGuiCond_Always);

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings;

		ImGui::Begin("##GeneratingOverlay", nullptr, flags);
		ImGui::TextUnformatted("Generating city...");
		ImGui::Separator();
		ImGui::TextUnformatted("Please wait.");
		ImGui::End();
	}

	// updates the OpenGL viewport and clears the frame buffers
	void viewPort_Setup(GLFWwindow *window)
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0.40f, 0.52f, 0.43f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// handles basic window input such as closing on escape
	void processInput(GLFWwindow *window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
	// GLFW error callback
	void error_callback(int error, const char *description)
	{
		std::cout << "Error: " << description << "\n";
	}

	// captures mouse wheel input for 2D zoom control
	void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
	{
		ImGuiIO &io = ImGui::GetIO();

		if (io.WantCaptureMouse)
			return;

		gScrollY += (float)yoffset;
	}

	// loads a 2D texture from disk and creates an OpenGL texture object
	GLuint LoadTexture2D(const std::string &path)
	{
		int w = 0, h = 0, channels = 0;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(path.c_str(), &w, &h, &channels, 0);
		if (!data)
			return 0;

		GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);
		return tex;
	}

	// deletes a texture safely and resets its handle
	void SafeDeleteTexture(GLuint &tex)
	{
		if (tex != 0)
		{
			glDeleteTextures(1, &tex);
			tex = 0;
		}
	}
	// handles GUI requests for applying or clearing the window texture
	void HandleWindowTextureRequests(Gui &gui, GLuint &windowTex, bool &useWindowTex)
	{
		std::string path;

		if (gui.ConsumeWindowTextureApply(path))
		{
			SafeDeleteTexture(windowTex);
			windowTex = LoadTexture2D(path);
			useWindowTex = (windowTex != 0);
		}

		if (gui.ConsumeWindowTextureClear())
		{
			SafeDeleteTexture(windowTex);
			useWindowTex = false;
		}
	}
	// handles GUI requests for road, sidewlak and ground textures
	void HandleSceneTextureRequests(Gui &gui,
									GLuint &roadTex, bool &useRoad,
									GLuint &sidewalkTex, bool &useSidewalk,
									GLuint &groundTex, bool &useGround)
	{
		std::string path;

		if (gui.ConsumeRoadTextureApply(path))
		{
			SafeDeleteTexture(roadTex);
			roadTex = LoadTexture2D(path);
			useRoad = (roadTex != 0);
		}
		if (gui.ConsumeRoadTextureClear())
		{
			SafeDeleteTexture(roadTex);
			useRoad = false;
		}

		if (gui.ConsumeSidewalkTextureApply(path))
		{
			SafeDeleteTexture(sidewalkTex);
			sidewalkTex = LoadTexture2D(path);
			useSidewalk = (sidewalkTex != 0);
		}
		if (gui.ConsumeSidewalkTextureClear())
		{
			SafeDeleteTexture(sidewalkTex);
			useSidewalk = false;
		}

		if (gui.ConsumeGroundTextureApply(path))
		{
			SafeDeleteTexture(groundTex);
			groundTex = LoadTexture2D(path);
			useGround = (groundTex != 0);
		}
		if (gui.ConsumeGroundTextureClear())
		{
			SafeDeleteTexture(groundTex);
			useGround = false;
		}
	}

	// handles GUI requests for urban, suburban and rural building textures
	void HandleBuildingTextureRequests(
		Gui &gui,
		GLuint &urbanTex, GLuint &suburbanTex, GLuint &ruralTex,
		bool &useUrban, bool &useSuburban, bool &useRural)
	{
		std::string path;

		// urban
		if (gui.ConsumeBuildingTextureApply(road::LotZone::Urban, path))
		{
			std::cout << "Loaded urbanTex=" << urbanTex << "\n";
			SafeDeleteTexture(urbanTex);
			urbanTex = LoadTexture2D(path);
			useUrban = (urbanTex != 0);
		}
		if (gui.ConsumeBuildingTextureClear(road::LotZone::Urban))
		{
			SafeDeleteTexture(urbanTex);
			useUrban = false;
		}

		// suburban
		if (gui.ConsumeBuildingTextureApply(road::LotZone::Suburban, path))
		{
			SafeDeleteTexture(suburbanTex);
			suburbanTex = LoadTexture2D(path);
			useSuburban = (suburbanTex != 0);
		}
		if (gui.ConsumeBuildingTextureClear(road::LotZone::Suburban))
		{
			SafeDeleteTexture(suburbanTex);
			useSuburban = false;
		}

		// rural
		if (gui.ConsumeBuildingTextureApply(road::LotZone::Rural, path))
		{
			SafeDeleteTexture(ruralTex);
			ruralTex = LoadTexture2D(path);
			useRural = (ruralTex != 0);
		}
		if (gui.ConsumeBuildingTextureClear(road::LotZone::Rural))
		{
			SafeDeleteTexture(ruralTex);
			useRural = false;
		}
	}

	// handles GUI requests for urban, suburban and rural roof textures
	void HandleRoofTextureRequests(
		Gui &gui,
		GLuint &urbanRoof, GLuint &suburbanRoof, GLuint &ruralRoof,
		bool &useUrbanRoof, bool &useSuburbanRoof, bool &useRuralRoof)
	{
		std::string path;

		if (gui.ConsumeRoofTextureApply(road::LotZone::Urban, path))
		{
			SafeDeleteTexture(urbanRoof);
			urbanRoof = LoadTexture2D(path);
			useUrbanRoof = (urbanRoof != 0);
		}
		if (gui.ConsumeRoofTextureClear(road::LotZone::Urban))
		{
			SafeDeleteTexture(urbanRoof);
			useUrbanRoof = false;
		}

		if (gui.ConsumeRoofTextureApply(road::LotZone::Suburban, path))
		{
			SafeDeleteTexture(suburbanRoof);
			suburbanRoof = LoadTexture2D(path);
			useSuburbanRoof = (suburbanRoof != 0);
		}
		if (gui.ConsumeRoofTextureClear(road::LotZone::Suburban))
		{
			SafeDeleteTexture(suburbanRoof);
			useSuburbanRoof = false;
		}

		if (gui.ConsumeRoofTextureApply(road::LotZone::Rural, path))
		{
			SafeDeleteTexture(ruralRoof);
			ruralRoof = LoadTexture2D(path);
			useRuralRoof = (ruralRoof != 0);
		}
		if (gui.ConsumeRoofTextureClear(road::LotZone::Rural))
		{
			SafeDeleteTexture(ruralRoof);
			useRuralRoof = false;
		}
	}

	// resolves the active city centre and radius for rendering camera logic
	CityContext ResolveCityContext(bool showRoads, const Gui &gui, const road::RoadParams &roadParams)
	{

		CityContext cityCTX;
		if (showRoads)
		{
			cityCTX.cityR = roadParams.cityRadius;
			cityCTX.centerXZ = glm::vec2(roadParams.cityCenter.x, roadParams.cityCenter.y);
		}
		else
		{
			cityCTX.cityR = gui.GetCityRadius();
			cityCTX.centerXZ = glm::vec2(0.0f, 0.0f);
		}
		return cityCTX;
	}

	// applies GUI-driven camera mode changes and resets requests
	bool UpdateCameraModeAndResetFromGui(Gui &gui, Camera &camera, const CityContext &cityCTX)
	{

		static bool prev3D = false;
		bool is3D = gui.Is3DEnabled();

		if (is3D != prev3D)
		{
			camera.Set3DEnabled(is3D, cityCTX.centerXZ, cityCTX.cityR);
			prev3D = is3D;
		}

		if (gui.ConsumeResetCamera())
		{
			camera.ResetViewToCity(cityCTX.centerXZ, cityCTX.cityR);
		}

		return is3D;
	}

	// computes frame timing, frambuffer size and the current view projection matrix
	FrameContext BeginFrameTimingAndVP(GLFWwindow *window, Camera &camera)
	{

		FrameContext frameCTX;

		static float last = (float)glfwGetTime();
		float now = (float)glfwGetTime();
		frameCTX.deltaTime = now - last;
		last = now;

		glfwGetFramebufferSize(window, &frameCTX.w, &frameCTX.h);
		frameCTX.aspect = (frameCTX.h == 0) ? 1.0f : (float)frameCTX.w / (float)frameCTX.h;

		frameCTX.viewProjection = camera.GetVP(frameCTX.aspect);
		return frameCTX;
	}

	// handles per-frame paltform input and viewport setup
	void HandlePlatformInputAndViewport(GLFWwindow *window)
	{

		processInput(window);
		viewPort_Setup(window);
	}

	// updates free-fly 3D camera controls using mouse + keyboard input
	void UpdateCamera3DControls(Camera &camera, GLFWwindow *window, float deltaTime)
	{

		bool rmbDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

		double mx = 0.0, my = 0.0;
		glfwGetCursorPos(window, &mx, &my);

		camera.OnMouseMove(mx, my, rmbDown);

		glfwSetInputMode(window, GLFW_CURSOR, rmbDown ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

		camera.UpdateFly3D(window, deltaTime);
	}

	// updates 2D pan and zoom camera controls
	void UpdateCamera2DControls(Camera &camera, GLFWwindow *window, float deltaTime)
	{

		camera.UpdatePanXZ(window, deltaTime);
		camera.ApplyScrollZoom(gScrollY);
		gScrollY = 0.0f;
	}

	// routes camera updates to either 2D or 3D controls
	void UpdateCameraPerFrame(Camera &camera, GLFWwindow *window, float deltaTime, bool is3D)
	{

		if (is3D)
		{
			UpdateCamera3DControls(camera, window, deltaTime);
		}
		else
		{
			UpdateCamera2DControls(camera, window, deltaTime);
		}
	}

	static std::vector<glm::vec3> StripBuildingTopCapQuads(const std::vector<glm::vec3> &buildingQuadVerts)
	{

		std::vector<glm::vec3> out;
		out.reserve(buildingQuadVerts.size());

		const size_t vertsPerBuilding = 5 * 4; // 20
		const size_t keepVerts = 4 * 4;		   // 16

		for (size_t i = 0; i + vertsPerBuilding <= buildingQuadVerts.size(); i += vertsPerBuilding)
		{
			// copy only wall quads
			out.insert(out.end(),
					   buildingQuadVerts.begin() + i,
					   buildingQuadVerts.begin() + i + keepVerts);
		}

		return out;
	}

	// exports the generated city geometry if requested through th eGUI
	void HandleExportIfRequested(Gui &gui, const CityContext &cityCTX, const GroundContext &ground,
								 const std::vector<glm::vec3> &roadHighwayTris,
								 const std::vector<glm::vec3> &roadStreetTris,
								 const std::vector<glm::vec3> &sidewalkTris,
								 const std::vector<glm::vec3> &buildingTriVerts,
								 const std::vector<glm::vec3> &buildingQuadVerts,
								 const std::vector<glm::vec3> &buildingRoofQuadVerts,
								 const std::vector<glm::vec3> &buildingRoofTriVerts,
								 const std::vector<glm::vec3> &buildingWindowQuadVerts,
								 const std::vector<glm::vec3> &buildingWindowTriVerts)
	{

		std::string dir, base;
		Gui::ExportFormat fmt;

		if (!gui.ConsumeExportRequest(dir, base, fmt))
		{
			return;
		}
		export3d::CityExportInput ex;
		ex.centerXZ = cityCTX.centerXZ;
		ex.halfW = ground.halfW;
		ex.halfH = ground.halfH;

		ex.roadHighwayTris = roadHighwayTris;
		ex.roadStreetTris = roadStreetTris;
		ex.sidewalkTris = sidewalkTris;
		ex.buildingTris = buildingTriVerts;

		if (gui.ExportBuildingRoofs())
			ex.buildingQuads = StripBuildingTopCapQuads(buildingQuadVerts);
		else
			ex.buildingQuads = buildingQuadVerts;

		if (gui.ExportBuildingRoofs())
			ex.buildingRoofQuads = buildingRoofQuadVerts;
		else
			ex.buildingRoofQuads.clear();

		if (gui.ExportBuildingRoofs())
		{
			ex.buildingRoofQuads = buildingRoofQuadVerts;
			ex.buildingRoofTris = buildingRoofTriVerts;
		}
		else
		{
			ex.buildingRoofQuads.clear();
			ex.buildingRoofTris.clear();
		}

		if (gui.ExportBuildingWindows())
			ex.buildingWindowQuads = buildingWindowQuadVerts;
		else
			ex.buildingWindowQuads.clear();

		std::string outPath;
		bool ok = false;

		if (fmt == Gui::ExportFormat::OBJ)
		{
			ok = export3d::CityExporter::ExportOBJ(dir, base, ex, &outPath);
		}

		gui.SetLastExportResult(ok, outPath);
	}

	// closes the application if the GUI requested a quit action
	void HandleQuitIfRequested(Gui &gui, GLFWwindow *window)
	{
		if (gui.WantsQuit())
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	// finalises and renders the ImGui frame
	void EndGuiFrame(Gui &gui)
	{
		gui.EndFrameGUI();
	}

	// begins a new ImGui frame and draws the GUI panels
	void BeginGuiFrame(Gui &gui)
	{
		gui.BeginFrameGUI();
		gui.DrawGUI();
	}

	// presents the rendered frame and processes window events
	void PresentAndPoll(GLFWwindow *window)
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// computes the directional light vector and light-space matrix for shadow mapping
	static void ComputeLightSpace(
		const CityContext &city,
		glm::vec3 &outLightDir,
		glm::mat4 &outLightSpace)
	{
		outLightDir = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.2f)); // sun direction

		const float r = city.cityR;
		const glm::vec2 c = city.centerXZ;
		const glm::vec3 center3(c.x, 0.0f, c.y);

		// light “camera”
		const glm::vec3 lightPos = center3 - outLightDir * (r * 2.5f);

		// ortho box covering the city
		const float ortho = r * 1.8f;
		const glm::mat4 lightView = glm::lookAt(lightPos, center3, glm::vec3(0, 1, 0));
		const glm::mat4 lightProj = glm::ortho(-ortho, ortho, -ortho, ortho, 0.1f, r * 6.0f);

		outLightSpace = lightProj * lightView;
	}

	// uploads lighting and shadow-map uniforms for the main lit shader
	static void SetupLightingAndShadowUniforms(
		Shader &litShader,
		ShadowMap &shadowMap,
		const glm::mat4 &lightSpace,
		const glm::vec3 &lightDir,
		bool is3D)
	{
		litShader.use();

		glUniformMatrix4fv(
			glGetUniformLocation(litShader.ID, "uLightSpace"),
			1, GL_FALSE, glm::value_ptr(lightSpace));

		glUniform3f(
			glGetUniformLocation(litShader.ID, "uLightDir"),
			lightDir.x, lightDir.y, lightDir.z);

		glUniform1i(
			glGetUniformLocation(litShader.ID, "uEnableShadows"),
			is3D ? 1 : 0);

		// bind shadow map on texture unit 1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadowMap.GetDepthTexture());
		glUniform1i(glGetUniformLocation(litShader.ID, "uShadowMap"), 1);
	}

	// draws the skybox as the background environment
	static void DrawSkyboxPass(
		Skybox &skybox,
		Camera &camera,
		float aspect,
		bool is3D)
	{

		glDisable(GL_CULL_FACE);

		if (is3D)
		{
			skybox.Draw(camera, aspect, is3D);
		}
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	// queues delayed generation and updates the loading overlay state
	static void HandleGenerationOverlayAndQueue(Gui &gui)
	{
		// show overlay first and delay generation to next frame
		if (gui.WantsGenerate())
		{
			gShowGeneratingOverlay = true;
			gGeneratePending = true;
		}

		// draw loading overlay on top of everything
		DrawGeneratingOverlay();
	}

}

// initialises the widnow, openGL, GUI, renderers and the main frame loop
int main(void)
{

	glfwSetErrorCallback(error_callback);

	// initialize
	if (!glfwInit())
		exit(EXIT_FAILURE);

	// request OpenGL 3.3 context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	// create the window 1280x800
	GLFWwindow *window = glfwCreateWindow(1280, 800, "ProceduralCityGenerator", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// wires mouse-wheel scroll events
	glfwSetScrollCallback(window, ScrollCallback);

	// makes OpenGl context current
	glfwMakeContextCurrent(window);

	// loads OpenGl functions with GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// GUI initialization
	Gui gui;
	if (!gui.Initialize_GUI(window, "#version 330"))
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// camera initialization
	sInit.camera.Set3DEnabled(false, glm::vec2(0.0f, 0.0f), gui.GetCityRadius());

	// shader paths
	Shader primShader(
		assets::Path("shaders/basic.vert"),
		assets::Path("shaders/basic.frag"));

	Shader lineShader(
		assets::Path("shaders/line.vert"),
		assets::Path("shaders/line.frag"));

	Shader buildingTexShader(
		assets::Path("shaders/building_tex.vert"),
		assets::Path("shaders/building_tex.frag"));

	Shader shadowDepthShader(
		assets::Path("shaders/shadow_depth.vert"),
		assets::Path("shaders/shadow_depth.frag"));

	Shader litShader(
		assets::Path("shaders/lit_shadow.vert"),
		assets::Path("shaders/lit_shadow.frag"));

	// all shutdowns
	auto ShutdownResources = [&]()
	{
		sInit.skybox.Shutdown();
		sRoads.highwayLines.Shutdown_Road();
		sRoads.streetLines.Shutdown_Road();
		sInit.primitives.Shutdown_Prim();
		sLots.lotLines.Shutdown_Road();
		sRoads.sidewalkLines.Shutdown_Road();
		sLots.gardenLines.Shutdown_Road();
		sLots.footprintLines.Shutdown_Road();
		sBuilding.buildingMesh.Shutdown();

		sBuilding.roofMesh.Shutdown();
		sBuilding.windowMesh.Shutdown();

		sRoads.roadMeshHighway.Shutdown();
		sRoads.roadMeshStreet.Shutdown();
		sRoads.sidewalkMesh.Shutdown();

		sTex.windowMeshTex.Shutdown();
		sTex.roadMeshHighwayTex.Shutdown();
		sTex.roadMeshStreetTex.Shutdown();
		sTex.sidewalkMeshTex.Shutdown();
		sTex.groundMeshTex.Shutdown();

		sTex.roofMeshUrban.Shutdown();
		sTex.roofMeshSuburban.Shutdown();
		sTex.roofMeshRural.Shutdown();

		sTex.buildingMeshUrban.Shutdown();
		sTex.buildingMeshSuburban.Shutdown();
		sTex.buildingMeshRural.Shutdown();

		SafeDeleteTexture(sTex.windowTex);
		SafeDeleteTexture(sTex.roadTex);
		SafeDeleteTexture(sTex.sidewalkTex);
		SafeDeleteTexture(sTex.groundTex);

		SafeDeleteTexture(sTex.urbanTex);
		SafeDeleteTexture(sTex.suburbanTex);
		SafeDeleteTexture(sTex.ruralTex);

		SafeDeleteTexture(sTex.roofUrbanTex);
		SafeDeleteTexture(sTex.roofSuburbanTex);
		SafeDeleteTexture(sTex.roofRuralTex);

		sInit.shadowMap.Shutdown();

		primShader.Shutdown();
		lineShader.Shutdown();
		buildingTexShader.Shutdown();
		shadowDepthShader.Shutdown();
		litShader.Shutdown();
		gui.ShutdownGUI();
	};

	// initialization block of all
	auto Fail = [&](const char *what)
	{
		std::cout << "Init failed: " << what << "\n";
		ShutdownResources();
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	};

	if (primShader.ID == 0)
		return Fail("primShader");

	if (lineShader.ID == 0)
		return Fail("lineShader");

	if (buildingTexShader.ID == 0)
		return Fail("buildingTexShader");

	if (shadowDepthShader.ID == 0)
		return Fail("shadowDepthShader");

	if (litShader.ID == 0)
		return Fail("litShader");

	// skybox initilization
	if (!sInit.skybox.Initialize(
			assets::Path("textures/SkyBox/Standard-Cube-Map").string()))
	{
		return Fail("skybox");
	}

	if (!sInit.primitives.Initialize_Prim())
	{
		return Fail("primitives");
	}

	// debug line renderers
	if (!sRoads.highwayLines.Initialize_Road())
		return Fail("highwayLines");
	if (!sRoads.streetLines.Initialize_Road())
		return Fail("streetLines");
	if (!sLots.lotLines.Initialize_Road())
		return Fail("lotLines");
	if (!sRoads.sidewalkLines.Initialize_Road())
		return Fail("sidewalkLines");
	if (!sLots.gardenLines.Initialize_Road())
		return Fail("gardenLines");
	if (!sLots.footprintLines.Initialize_Road())
		return Fail("footprintLines");

	// non-textured meshes
	if (!sBuilding.buildingMesh.Initialize())
		return Fail("buildingMesh");
	if (!sRoads.roadMeshHighway.Initialize())
		return Fail("roadMeshHighway");
	if (!sRoads.roadMeshStreet.Initialize())
		return Fail("roadMeshStreet");
	if (!sRoads.sidewalkMesh.Initialize())
		return Fail("sidewalkMesh");

	// textured building meshes (urban/suburban/rural)
	if (!sTex.buildingMeshUrban.Initialize())
		return Fail("buildingMeshUrban");
	if (!sTex.buildingMeshSuburban.Initialize())
		return Fail("buildingMeshSuburban");
	if (!sTex.buildingMeshRural.Initialize())
		return Fail("buildingMeshRural");

	// details
	if (!sBuilding.roofMesh.Initialize())
		return Fail("roofMesh");
	if (!sBuilding.windowMesh.Initialize())
		return Fail("windowMesh");

	// textured windows (alpha windows)
	if (!sTex.windowMeshTex.Initialize())
		return Fail("windowMeshTex");

	// textured roofs
	if (!sTex.roofMeshUrban.Initialize())
		return Fail("roofMeshUrban");
	if (!sTex.roofMeshSuburban.Initialize())
		return Fail("roofMeshSuburban");
	if (!sTex.roofMeshRural.Initialize())
		return Fail("roofMeshRural");

	// road, sidewalk and ground texture
	if (!sTex.roadMeshHighwayTex.Initialize())
		return Fail("roadMeshHighwayTex");
	if (!sTex.roadMeshStreetTex.Initialize())
		return Fail("roadMeshStreetTex");
	if (!sTex.sidewalkMeshTex.Initialize())
		return Fail("sidewalkMeshTex");
	if (!sTex.groundMeshTex.Initialize())
		return Fail("groundMeshTex");

	if (!sInit.shadowMap.Init(4096))
		return Fail("shadowMap");

	std::cout << "All renderers initialized OK.\n";

	// openGL state + error reporting
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	enableReportGlErrors();

	// main application frame loop
	while (!glfwWindowShouldClose(window))
	{
		// GUI
		BeginGuiFrame(gui);

		// texture handling
		HandleBuildingTextureRequests(gui, sTex.urbanTex, sTex.suburbanTex, sTex.ruralTex, sTex.useUrban, sTex.useSuburban, sTex.useRural);

		// building window texture handling
		HandleWindowTextureRequests(gui, sTex.windowTex, sTex.useWindowTex);

		// building roof texture handling
		HandleRoofTextureRequests(gui, sTex.roofUrbanTex, sTex.roofSuburbanTex, sTex.roofRuralTex, sTex.useRoofUrban, sTex.useRoofSuburban, sTex.useRoofRural);

		// road, sidewalk and ground handling
		HandleSceneTextureRequests(gui, sTex.roadTex, sTex.useRoadTex, sTex.sidewalkTex, sTex.useSidewalkTex, sTex.groundTex, sTex.useGroundTex);

		// choose city radius and center based on wheter roads are shown
		CityContext city = ResolveCityContext(sRoads.showRoads, gui, sRoads.roadParams);

		glm::vec3 lightDir;
		glm::mat4 lightSpace;
		ComputeLightSpace(city, lightDir, lightSpace);

		// camera mode toggle + reset
		const bool is3D = UpdateCameraModeAndResetFromGui(gui, sInit.camera, city);

		// camera  and ground adjustment
		FrameContext frame = BeginFrameTimingAndVP(window, sInit.camera);

		rendering::RenderShadowPassIf3D(
			is3D,
			gui,
			sInit.shadowMap,
			shadowDepthShader,
			lightSpace,
			frame,
			sRoads,
			sBuilding,
			sTex);

		SetupLightingAndShadowUniforms(litShader, sInit.shadowMap, lightSpace, lightDir, is3D);

		// input + viewport
		HandlePlatformInputAndViewport(window);

		// camera per-frame update + controls
		UpdateCameraPerFrame(sInit.camera, window, frame.deltaTime, is3D);

		// draws 3D meshes (only when is3D)
		rendering::Draw3DMeshesIfEnabled(
			gui,
			is3D,
			litShader,
			frame.viewProjection,
			sRoads,
			sBuilding,
			sTex);

		DrawSkyboxPass(sInit.skybox, sInit.camera, frame.aspect, is3D);

		glDisable(GL_CULL_FACE);

		// ground
		GroundContext ground = rendering::DrawGroundAndGetExtents(
			sInit.primitives,
			primShader,
			litShader,
			frame.viewProjection,
			city,
			sTex);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// export request
		HandleExportIfRequested(gui, city, ground, sRoads.roadHighwayTris, sRoads.roadStreetTris, sRoads.sidewalkTris, sBuilding.buildingTriVerts, sBuilding.buildingQuadVerts, sBuilding.buildingRoofQuadVerts, sBuilding.buildingRoofTriVerts, sBuilding.buildingWindowQuadVerts, sBuilding.buildingWindowTriVerts);

		// debug lines
		rendering::DrawDebugLinesIfEnabled(
			gui,
			lineShader,
			frame.viewProjection,
			sRoads,
			sLots);

		// show overlay first and delay generation to next frame
		HandleGenerationOverlayAndQueue(gui);

		// draw loading overlay on top of everything
		DrawGeneratingOverlay();

		HandleQuitIfRequested(gui, window);

		EndGuiFrame(gui);

		if (gui.ConsumeGuiRecreateRequest())
		{
			gui.ShutdownGUI();

			if (!gui.Initialize_GUI(window, "#version 330"))
				return Fail("GUI reinitialization");
		}

		PresentAndPoll(window);

		// run generation after overlay frame is presented
		if (gGeneratePending)
		{
			gGeneratePending = false;

			generation::GenerateCityNow(gui, sRoads, sLots, sBuilding, sTex);

			gShowGeneratingOverlay = false;
		}
	}

	ShutdownResources();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
