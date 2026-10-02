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
#include "Textures/TextureUtils.h"
#include "Textures/TextureRequests.h"
#include "Application/CameraController.h"
#include "Application/WindowController.h"
#include "Export/CityExportController.h"
#include "Textures/TextureContext.h"

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

		textures::SafeDeleteTexture(sTex.windowTex);
		textures::SafeDeleteTexture(sTex.roadTex);
		textures::SafeDeleteTexture(sTex.sidewalkTex);
		textures::SafeDeleteTexture(sTex.groundTex);

		textures::SafeDeleteTexture(sTex.urbanTex);
		textures::SafeDeleteTexture(sTex.suburbanTex);
		textures::SafeDeleteTexture(sTex.ruralTex);

		textures::SafeDeleteTexture(sTex.roofUrbanTex);
		textures::SafeDeleteTexture(sTex.roofSuburbanTex);
		textures::SafeDeleteTexture(sTex.roofRuralTex);

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
		textures::HandleBuildingTextureRequests(gui, sTex.urbanTex, sTex.suburbanTex, sTex.ruralTex, sTex.useUrban, sTex.useSuburban, sTex.useRural);

		// building window texture handling
		textures::HandleWindowTextureRequests(gui, sTex.windowTex, sTex.useWindowTex);

		// building roof texture handling
		textures::HandleRoofTextureRequests(gui, sTex.roofUrbanTex, sTex.roofSuburbanTex, sTex.roofRuralTex, sTex.useRoofUrban, sTex.useRoofSuburban, sTex.useRoofRural);

		// road, sidewalk and ground handling
		textures::HandleSceneTextureRequests(gui, sTex.roadTex, sTex.useRoadTex, sTex.sidewalkTex, sTex.useSidewalkTex, sTex.groundTex, sTex.useGroundTex);

		// choose city radius and center based on wheter roads are shown
		CityContext city = ResolveCityContext(sRoads.showRoads, gui, sRoads.roadParams);

		glm::vec3 lightDir;
		glm::mat4 lightSpace;
		rendering::ComputeLightSpace(city, lightDir, lightSpace);

		// camera mode toggle + reset
		const bool is3D =
			application::UpdateCameraModeAndResetFromGui(gui, sInit.camera, city);

		// camera  and ground adjustment
		FrameContext frame =
			application::BeginFrameTimingAndVP(window, sInit.camera);

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

		rendering::SetupLightingAndShadowUniforms(litShader, sInit.shadowMap, lightSpace, lightDir, is3D);

		// input + viewport
		application::HandlePlatformInputAndViewport(window);

		// camera per-frame update + controls
		application::UpdateCameraPerFrame(
			sInit.camera, window, frame.deltaTime, is3D, gScrollY);

		// draws 3D meshes (only when is3D)
		rendering::Draw3DMeshesIfEnabled(
			gui,
			is3D,
			litShader,
			frame.viewProjection,
			sRoads,
			sBuilding,
			sTex);

		rendering::DrawSkyboxPass(sInit.skybox, sInit.camera, frame.aspect, is3D);

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
		export3d::HandleExportIfRequested(gui, city, ground, sRoads, sBuilding);

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
