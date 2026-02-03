#define GLFW_INCLUDE_NONE

//externals
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
#include <chrono>

//main project dependencies
#include "Primitives.h"
#include "Camera.h"
#include "Road/RoadGenerator.h"
#include "Road/RoadParams.h"
#include "Road/LineRenderer.h"
#include "Road/LotSubdivision.h"
#include "Road/LotParams.h"
#include "Road/LotTypes.h"
#include "Road/SideWalkGenerator.h"
#include "Road/BuildingRenderer.h"
#include "Road/RoadSurfaceGenerator.h"
#include "Export/CityExporter.h"
#include "Gui.h"
#include <Shader.h>
#include "Skybox.h"


//global Y axis variable for zooming in/out control.
static float gScrollY = 0.0f;
double gLastGenerationMs = 0.0;

//struct definitions
struct CityContext
{
	float cityR = 0.0f;
	glm::vec2 centerXZ =glm::vec2(0.0f, 0.0f);
};

struct FrameContext
{
	float deltaTime = 0.0f;
	int w = 1;
	int h = 1;
	float aspect = 1.0f;
	glm::mat4 viewProjection = glm::mat4(1.0f);
};

struct GroundContext 
{
	float margin = 20.0f;
	float halfW = 0.0f;
	float halfH = 0.0f;
};


namespace
{
	//print helper
	void PrintRoadParams(const road::RoadParams& rp)
	{
		std::cout
		<< "[GUI] cityRadius=" << rp.cityRadius
		<< " maxIterations=" << rp.maxIterations
		<< " maxStreets=" << rp.maxStreetSegments   
		<< " maxHighways=" << rp.maxHighwaySegments 
		<< "\n";
	}

	void BuildAndUploadRoadAndSidewalkMeshes(
        const road::RoadNetwork& roadNet,
        const road::RoadParams& roadParams,
        std::vector<glm::vec3>& roadHighwayTris,
        std::vector<glm::vec3>& roadStreetTris,
        std::vector<glm::vec3>& sidewalkTris,
        road::BuildingRenderer& roadMeshHighway,
        road::BuildingRenderer& roadMeshStreet,
        road::BuildingRenderer& sidewalkMesh)
    {
        // build 3D road + sidewalk slabs
        const float roadBaseY = 0.02f;
        const float roadH     = 0.02f;

        // sidewalk sits above road
        const float sidewalkBaseY = roadBaseY + roadH;
        const float sidewalkH     = 0.08f;

        // trim
        const float trimExtra = 0.0f;

        road::BuildRoadSurfaceTriVerts(
            roadNet, roadParams,
            roadHighwayTris, roadStreetTris,
            roadBaseY, roadH);

        road::BuildSidewalkSurfaceTriVerts(
            roadNet, roadParams,
            sidewalkTris,
            sidewalkBaseY, sidewalkH,
            trimExtra, false);

        roadMeshHighway.Upload(roadHighwayTris);
        roadMeshStreet.Upload(roadStreetTris);
        sidewalkMesh.Upload(sidewalkTris);

        std::cout << "Road tris => highway: " << roadHighwayTris.size()
                  << " street: " << roadStreetTris.size()
                  << " sidewalks: " << sidewalkTris.size()
                  << "\n";
    }

	void BuildAndUploadRoadLineVerts(const road::RoadNetwork& roadNet, road::LineRenderer& highwayLines,
									road::LineRenderer& streetLines)
	{
		std::vector<glm::vec3> highwayVerts;
		std::vector<glm::vec3> streetVerts;

		highwayVerts.reserve(roadNet.Segments().size() * 2);
		streetVerts.reserve(roadNet.Segments().size() * 2);

		for (const auto &seg : roadNet.Segments())
		{
			const auto &A = roadNet.Nodes().at(seg.a - 1).pos;
			const auto &B = roadNet.Nodes().at(seg.b - 1).pos;

			glm::vec3 a3(A.x, 0.05f, A.y);
			glm::vec3 b3(B.x, 0.05f, B.y);

			if (seg.type == road::RoadType::Highway)
			{
				highwayVerts.push_back(a3);
				highwayVerts.push_back(b3);
			}
			else
			{
				streetVerts.push_back(a3);
				streetVerts.push_back(b3);
			}
		}
		highwayLines.Upload(highwayVerts);
		streetLines.Upload(streetVerts);
	}
	road::LotParams MakeLotParamsFromRoadParams(const Gui& gui, const road::RoadParams& roadParams)
	{
		road::LotParams lotParams = gui.GetLotParams();
		lotParams.seed = roadParams.seed;
		lotParams.cityRadius = roadParams.cityRadius;

		lotParams.streetHalfWidth = roadParams.streetHalfWidth;
		lotParams.highwayHalfWidth = roadParams.highwayHalfWidth;

		lotParams.sidewalkWidth = roadParams.sidewalkWidth;
		lotParams.sidewalkGap = roadParams.sidewalkGap;

		return lotParams;
	}

	void PrintGardenPipelineStats(const road::LotCollection lots, const  road::LotParams& lotParams){
		
		int nUrban = 0, nSub = 0, nRural = 0;
		int passZone = 0, passArea = 0, passDepth = 0, passAll = 0;
		
		for (const auto& l : lots.lots)
		{
			if (l.zone == road::LotZone::Urban) nUrban++;
			else if (l.zone == road::LotZone::Suburban) nSub++;
			else nRural++;

			// zone
			if (l.zone != road::LotZone::Suburban) continue;
			passZone++;

			// area
			if (l.area < lotParams.minLotAreaForGarden) continue;
			passArea++;

			// depth feasibility (recompute from boundary)
			if (l.boundary.size() < 4) continue;
			float fullDepth = glm::length(l.boundary[3] - l.boundary[0]);
			float gardenDepth = fullDepth * lotParams.gardenBackRatio;

			if (gardenDepth < lotParams.minGardenDepth) continue;
			if ((fullDepth - gardenDepth) < 0.5f) continue;
			passDepth++;

			// actual result
			if (l.hasGarden) passAll++;
		}

		std::cout << "Zones => Urban: " << nUrban << " Suburban: " << nSub << " Rural: " << nRural << "\n";
		std::cout << "Garden pipeline => passZone: " << passZone
				<< " passArea: " << passArea
				<< " passDepth: " << passDepth
				<< " hasGarden: " << passAll << "\n";

		float minA = 1e9f, maxA = 0.0f;
		for (const auto& l : lots.lots)
		{
			minA = std::min(minA, l.area);
			maxA = std::max(maxA, l.area);
		}
		std::cout << "Lot area range: min=" << minA << " max=" << maxA
				<< " (threshold=" << lotParams.minLotAreaForGarden << ")\n";


	}

	void BuildAndUploadLotDerivedGeometry(
		const road::RoadNetwork& roadNet,
		const road::RoadParams& roadParams,
		const road::LotCollection& lots,
		std::vector<glm::vec3>& lotLineVerts,
		std::vector<glm::vec3>& sidewalkLineVerts,
		std::vector<glm::vec3>& gardenLineVerts,
		std::vector<glm::vec3>& footprintLineVerts,
		std::vector<glm::vec3>& buildingTriVerts,
		road::LineRenderer& lotLines,
		road::LineRenderer& sidewalkLines,
		road::LineRenderer& gardenLines,
		road::LineRenderer& footprintLines,
		road::BuildingRenderer& buildingMesh)
	{

		const float baseY = 0.03f;
		const float floorH = 0.35f;



		road::BuildLotLineVerts(lots, lotLineVerts, 0.02f);
		road::BuildSidewalkLineVerts(roadNet, roadParams, sidewalkLineVerts, 0.06f);
		road::BuildGardenLineVerts(lots, gardenLineVerts, 0.021f);
		road::BuildFootprintLineVerts(lots, footprintLineVerts, 0.022f);
		road::BuildBuildingTriVerts(lots, buildingTriVerts, baseY, floorH);

	
		std::cout << "Building tri verts: " << buildingTriVerts.size() << "\n";



		lotLines.Upload(lotLineVerts);
		sidewalkLines.Upload(sidewalkLineVerts);
		gardenLines.Upload(gardenLineVerts);
		footprintLines.Upload(footprintLineVerts);
		buildingMesh.Upload(buildingTriVerts);


		std::cout << "Lots count: " << lots.lots.size() << "\n";
		std::cout << "Lot verts: " << lotLineVerts.size() << "\n";
		std::cout << "Garden verts: " << gardenLineVerts.size() << "\n";
	}


	void viewPort_Setup(GLFWwindow *window)
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	}

	void processInput(GLFWwindow *window)
	{
		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}

	void error_callback(int error, const char *description)
	{
		std::cout << "Error: " <<  description << "\n";
	}

	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		ImGuiIO& io = ImGui::GetIO();


		if (io.WantCaptureMouse)
			return;


		gScrollY += (float)yoffset;
	}
	//city context
	CityContext ResolveCityContext(bool showRoads, const Gui& gui, const road::RoadParams& roadParams) {

		CityContext cityCTX;
		if (showRoads) {
			cityCTX.cityR = roadParams.cityRadius;
			cityCTX.centerXZ = glm::vec2(roadParams.cityCenter.x, roadParams.cityCenter.y);
		} else {
			cityCTX.cityR = gui.GetCityRadius();
			cityCTX.centerXZ = glm::vec2(0.0f, 0.0f); 
		}
		return cityCTX;
	}
	//Camera toggle + reset
	bool UpdateCameraModeAndResetFromGui(Gui& gui, Camera& camera, const CityContext& cityCTX) {

		static bool prev3D = false;
		bool is3D = gui._3DEnabled(); 

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

	//compute deltaTime + framebuffer size + viewProjection matrix
	FrameContext BeginFrameTimingAndVP(GLFWwindow* window, Camera& camera) {

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

	void HandlePlatformInputAndViewport(GLFWwindow* window) {

		processInput(window);
		viewPort_Setup(window);
	}

	//camera controls
	void UpdateCamera3DControls(Camera& camera, GLFWwindow* window, float deltaTime) {
			
			bool rmbDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

			double mx = 0.0, my =0.0;
			glfwGetCursorPos(window, &mx, &my);

			camera.OnMouseMove(mx, my, rmbDown);

			glfwSetInputMode(window, GLFW_CURSOR, rmbDown ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

			camera.UpdateFly3D(window, deltaTime);
	}

	void UpdateCamera2DControls(Camera& camera, GLFWwindow* window, float deltaTime) {
			
			camera.UpdatePanXZ(window, deltaTime);
			camera.ApplyScrollZoom(gScrollY);
			gScrollY = 0.0f;
	}

	void UpdateCameraPerFrame(Camera& camera, GLFWwindow* window, float deltaTime, bool is3D) {
		
		camera.Update(deltaTime);

		if (is3D) {
			UpdateCamera3DControls(camera, window, deltaTime);
		} else {
			UpdateCamera2DControls(camera, window, deltaTime);
		}
	}

	void Draw3DMeshesIfEnabled (bool is3D, Shader& lineShader, const glm::mat4& viewProjection,
										road::BuildingRenderer& roadMeshStreet,
										road::BuildingRenderer& roadMeshHighway,
										road::BuildingRenderer& sidewalkMesh,
										road::BuildingRenderer& buildingMesh) 
	{
			if (!is3D)
			{
				return;
			}

			glDisable(GL_CULL_FACE); 
			lineShader.use();

			//streets
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.12f, 0.12f, 0.12f);
			roadMeshStreet.Draw(lineShader, viewProjection);

			//highways
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.07f, 0.07f, 0.07f);
			roadMeshHighway.Draw(lineShader, viewProjection);

			//sidewalks
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.70f, 0.70f, 0.70f);
			sidewalkMesh.Draw(lineShader, viewProjection);

			//buildings
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.75f, 0.75f, 0.78f);
			buildingMesh.Draw(lineShader, viewProjection);


	}
	//ground and returns for export
	GroundContext DrawGroundAndGetExtents(Primitives& primitives, Shader& primShader, const glm::mat4& viewProjection, const CityContext& cityCTX) {


		GroundContext g;
		// extra ground area beyond city boundries 
		g.margin = 20.0f;

		// ground half extents based on city size
		g.halfW = cityCTX.cityR + g.margin;
		g.halfH = cityCTX.cityR + g.margin;

		primitives.DrawGround(primShader, viewProjection, cityCTX.centerXZ, g.halfW, g.halfH);
		return g;

	}

	void HandleExportIfRequested(Gui& gui, const CityContext& cityCTX, const GroundContext& ground,
										const std::vector<glm::vec3>& roadHighwayTris,
										const std::vector<glm::vec3>& roadStreetTris,
										const std::vector<glm::vec3>& sidewalkTris,
										const std::vector<glm::vec3>& buildingTriVerts)
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
			ex.roadStreetTris  = roadStreetTris;
			ex.sidewalkTris    = sidewalkTris;
			ex.buildingTris    = buildingTriVerts;

			std::string outPath;
			bool ok = false;

			if (fmt == Gui::ExportFormat::OBJ)
			{
				ok = export3d::CityExporter::ExportOBJ(dir, base, ex, &outPath);
			}

			gui.SetLastExportResult(ok, outPath);
		
	}

	void DrawDebugLinesIfEnabled(bool showRoads, Gui& gui, Shader& lineShader, const glm::mat4& viewProjection,
										road::LineRenderer& highwayLines, road::LineRenderer& streetLines,
										road::LineRenderer& lotLines, road::LineRenderer& sidewalkLines,
										road::LineRenderer& gardenLines, road::LineRenderer& footprintLines)
	{
		
		if (!showRoads)
		{
			return;
		}

		lineShader.use();
		
		//highways
		glLineWidth(4.0f);
		glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
		highwayLines.Draw(lineShader, viewProjection);

		//streets
		glLineWidth(1.0f);
		glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
		streetLines.Draw(lineShader, viewProjection);

		//lots 
		if (gui.ShowLotDebug())
		{
			glLineWidth(2.0f);
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.1f, 0.4f, 1.0f);
			lotLines.Draw(lineShader, viewProjection);
			glLineWidth(1.0f);
		}

		//sidewalks
		if (gui.ShowSideWalks())
		{
			glLineWidth(2.0f);	
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.65f, 0.65f, 0.65f);
			sidewalkLines.Draw(lineShader, viewProjection);
			glLineWidth(1.0f);
		}

		//gardens
		if (gui.ShowGardens())
		{
			glLineWidth(2.0f);
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.1f, 0.9f, 0.2f);
			gardenLines.Draw(lineShader, viewProjection);
			glLineWidth(1.0f);
		}

		//building footprints
		if (gui.ShowFootprints())
		{
			glLineWidth(2.0f);
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.95f, 0.65f, 0.15f);
			footprintLines.Draw(lineShader, viewProjection);
			glLineWidth(1.0f);
		}
	}

	void HandleQuitIfRequested(Gui& gui, GLFWwindow* window)
	{
		if (gui.WantsQuit())
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	void EndGuiFrame(Gui& gui)
	{
		gui.EndFrameGUI();

	}

	void BeginGuiFrame(Gui& gui)
	{
		gui.BeginFrameGUI();
		gui.DrawGUI();
	}

	void PresentAndPoll(GLFWwindow* window)
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}



	//main generate function
	void GenerateCityIfRequested(
			Gui& gui,
			bool& showRoads,
			road::RoadGenerator& roadGen,
			road::LotSubdivision& lotGen,
			road::RoadParams& roadParams,
			road::RoadNetwork& roadNet,
			road::LotCollection& lots,

			// 3D meshes:
			road::BuildingRenderer& roadMeshHighway,
			road::BuildingRenderer& roadMeshStreet,
			road::BuildingRenderer& sidewalkMesh,
			road::BuildingRenderer& buildingMesh,

			// Line renderers:
			road::LineRenderer& highwayLines,
			road::LineRenderer& streetLines,
			road::LineRenderer& lotLines,
			road::LineRenderer& sidewalkLines,
			road::LineRenderer& gardenLines,
			road::LineRenderer& footprintLines,

			// Geometry buffers:
			std::vector<glm::vec3>& roadHighwayTris,
			std::vector<glm::vec3>& roadStreetTris,
			std::vector<glm::vec3>& sidewalkTris,
			std::vector<glm::vec3>& lotLineVerts,
			std::vector<glm::vec3>& sidewalkLineVerts,
			std::vector<glm::vec3>& gardenLineVerts,
			std::vector<glm::vec3>& footprintLineVerts,
			std::vector<glm::vec3>& buildingTriVerts)
	{
			if (!gui.WantsGenerate())
				return;

			

			// 1) Pull params from GUI
			roadParams = gui.GetParams();
			PrintRoadParams(roadParams);

			//start timer
			using Clock = std::chrono::high_resolution_clock;
			auto t0 = Clock::now();

			// 2) Generate road network
			roadNet = roadGen.Generate(roadParams);

			// 3) Build & upload road + sidewalk slabs
			BuildAndUploadRoadAndSidewalkMeshes(
				roadNet, roadParams,
				roadHighwayTris, roadStreetTris, sidewalkTris,
				roadMeshHighway, roadMeshStreet, sidewalkMesh);

			// 4) Build & upload highway/street line renderers
			showRoads = true;
			BuildAndUploadRoadLineVerts(roadNet, highwayLines, streetLines);

			// 5) Lot generation
			road::LotParams lotParams = MakeLotParamsFromRoadParams(gui, roadParams);
			lots = lotGen.GenerateLots(roadNet, lotParams);

			// 6) Debug stats
			PrintGardenPipelineStats(lots, lotParams);

			// 7) Build derived geometry (lots/sidewalk/gardens/footprints/buildings) + upload
			BuildAndUploadLotDerivedGeometry(
				roadNet, roadParams, lots,
				lotLineVerts, sidewalkLineVerts, gardenLineVerts, footprintLineVerts, buildingTriVerts,
				lotLines, sidewalkLines, gardenLines, footprintLines,
				buildingMesh);

			auto t1 = Clock::now();
			gLastGenerationMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
			gui.SetLastGenerationMs(gLastGenerationMs);
	}

}





int main(void)
{

	glfwSetErrorCallback(error_callback);

	//initialize
	if (!glfwInit())
		exit(EXIT_FAILURE);

	//request OpenGL 3.3 context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	//fullscreen
	//GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	//const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	//create the window 1280x800
	GLFWwindow *window = glfwCreateWindow(1280, 800, "LiteCityForge", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	//wires mouse-wheel scroll events
	glfwSetScrollCallback(window, ScrollCallback);
	//makes OpenGl context current
	glfwMakeContextCurrent(window);

	//loads OpenGl functions with GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	//GUI initialization
	Gui gui;
	if (!gui.Initialize_GUI(window, "#version 330"))
	{
		return -1;
	}

	//ground variables
	Primitives primitives;
	primitives.Initialize_Prim();

	//camera variables
	Camera camera;
	camera.Set3DEnabled(false, glm::vec2(0.0f, 0.0f), gui.GetCityRadius());


	//road variables
	bool showRoads = false;
	road::RoadGenerator roadGen;
	road::RoadParams roadParams;
	road::RoadNetwork roadNet;
	road::LineRenderer highwayLines;
	road::LineRenderer streetLines;
	std::vector<glm::vec3> roadLineVerts;

	//lot variables
	road::LotSubdivision lotGen;
	road::LotCollection lots;
	road::LineRenderer lotLines;
	std::vector<glm::vec3> lotLineVerts;

	//sidewalk variables
	road::LineRenderer sidewalkLines;
	std::vector<glm::vec3> sidewalkLineVerts;

	//garden variables
	road::LineRenderer gardenLines;
	std::vector<glm::vec3> gardenLineVerts;

	//building footprint variables
	road::LineRenderer footprintLines;
	std::vector<glm::vec3> footprintLineVerts;

	//building mesh variable
	road::BuildingRenderer buildingMesh;
	std::vector<glm::vec3> buildingTriVerts;

	//roads and sidewalks mesh variables
	road::BuildingRenderer roadMeshHighway;
	road::BuildingRenderer roadMeshStreet;
	road::BuildingRenderer sidewalkMesh;

	std::vector<glm::vec3> roadHighwayTris;
	std::vector<glm::vec3> roadStreetTris;
	std::vector<glm::vec3> sidewalkTris;

	
	//two shaders: one for primitives (such as ground), the other is for line rendering of triangles
	Shader primShader("../include/basic.vert", "../include/basic.frag");
	Shader lineShader("../include/line.vert", "../include/line.frag");

	//Skybox initilization
	Skybox skybox;
	if (!skybox.Initialize("../assets/textures/SkyBox/Standard-Cube-Map"))
	{
		std::cout << "Skybox init failed. \n";
	}

	//initialization block of all 
	if (!highwayLines.Initialize_Road() || !streetLines.Initialize_Road() || 
		!lotLines.Initialize_Road() || !sidewalkLines.Initialize_Road()
		|| !gardenLines.Initialize_Road() || !footprintLines.Initialize_Road()
		|| !buildingMesh.Initialize() || !roadMeshHighway.Initialize() || !roadMeshStreet.Initialize()
		|| !sidewalkMesh.Initialize())
	{
		std::cout << "Failed to init LineRenderer for highways/streets/lots/roads/garden/footprints\n";
		return -1;
	}

	//openGL state + error reporting
	glEnable(GL_DEPTH_TEST);
	enableReportGlErrors();


//the frame loop
while (!glfwWindowShouldClose(window))
{
	//GUI
	BeginGuiFrame(gui);

	//choose city radius and center based on wheter roads are shown
	CityContext city = ResolveCityContext(showRoads, gui, roadParams);

	//camera mode toggle + reset
	const bool is3D = UpdateCameraModeAndResetFromGui(gui, camera, city);
	
	// camera  and ground adjustment
	FrameContext frame = BeginFrameTimingAndVP(window, camera);

	//input + viewport
	HandlePlatformInputAndViewport(window);

	//Draw skybox scene
	skybox.Draw(camera, frame.aspect, is3D);

	//camera per-frame update + controls
	UpdateCameraPerFrame(camera, window, frame.deltaTime, is3D);

	//draws 3D meshes (only when is3D)
	Draw3DMeshesIfEnabled(is3D, lineShader, frame.viewProjection, roadMeshStreet, roadMeshHighway, sidewalkMesh, buildingMesh);

	//ground and allows export
	GroundContext ground = DrawGroundAndGetExtents(primitives, primShader, frame.viewProjection, city);

	//export request
	HandleExportIfRequested(gui, city, ground, roadHighwayTris, roadStreetTris, sidewalkTris, buildingTriVerts);

	//debug lines
	DrawDebugLinesIfEnabled(showRoads, gui, lineShader, frame.viewProjection, highwayLines, streetLines, lotLines, sidewalkLines, gardenLines, footprintLines);

	GenerateCityIfRequested(gui, showRoads, roadGen, lotGen, roadParams, roadNet, lots,
    						roadMeshHighway, roadMeshStreet, sidewalkMesh, buildingMesh,
    						highwayLines, streetLines, lotLines, sidewalkLines, gardenLines, footprintLines,
    						roadHighwayTris, roadStreetTris, sidewalkTris,
    						lotLineVerts, sidewalkLineVerts, gardenLineVerts, footprintLineVerts, buildingTriVerts);

	HandleQuitIfRequested(gui, window);

	EndGuiFrame(gui);

	PresentAndPoll(window);

}
	//all shutdowns
	skybox.Shutdown();
	highwayLines.Shutdown_Road();
	streetLines.Shutdown_Road();
	primitives.Shutdown_Prim();
	lotLines.Shutdown_Road();
	sidewalkLines.Shutdown_Road();
	gardenLines.Shutdown_Road();
	footprintLines.Shutdown_Road();
	buildingMesh.Shutdown();
	gui.ShutdownGUI();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}






