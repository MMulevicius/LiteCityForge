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
#include "Rendering/ShadowMap.h"


//main project dependencies
#include "Core/Primitives.h"
#include "Core/Camera.h"
#include "Road/RoadGenerator.h"
#include "Road/RoadParams.h"
#include "Rendering/LineRenderer.h"
#include "Lots/LotSubdivision.h"
#include "Lots/LotParams.h"
#include "Lots/LotTypes.h"
#include "Road/SideWalkGenerator.h"
#include "Rendering/BuildingRenderer.h"
#include "Road/RoadSurfaceGenerator.h"
#include "Export/CityExporter.h"
#include "GUI/Gui.h"
#include <Rendering/Shader.h>
#include "Rendering/Skybox.h"


//global Y axis variable for zooming in/out control.
static float gScrollY = 0.0f;
double gLastGenerationMs = 0.0;

//controls the delayed city-generation overlay workflow
static bool gShowGeneratingOverlay = false;
static bool gGeneratePending       = false;

//context structs used to group related runtime state
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

struct BuildingContext
{
	//building mesh variable
	road::BuildingRenderer buildingMesh;
	road::BuildingRenderer roofMesh;
	road::BuildingRenderer windowMesh;

	//building detail tris
	std::vector<glm::vec3> buildingTriVerts;
	std::vector<glm::vec3> buildingRoofTriVerts;
	std::vector<glm::vec3> buildingWindowTriVerts;

	//building detail quads
	std::vector<glm::vec3> buildingQuadVerts;
	std::vector<glm::vec3> buildingRoofQuadVerts;
	std::vector<glm::vec3> buildingWindowQuadVerts;

};

struct RoadContext
{
	//road variables
	bool showRoads = false;
	road::RoadGenerator roadGen;
	road::RoadParams roadParams;
	road::RoadNetwork roadNet;
	road::LineRenderer highwayLines;
	road::LineRenderer streetLines;
	std::vector<glm::vec3> roadLineVerts;

	//roads and sidewalks mesh variables
	road::BuildingRenderer roadMeshHighway;
	road::BuildingRenderer roadMeshStreet;
	road::BuildingRenderer sidewalkMesh;

	std::vector<glm::vec3> roadHighwayTris;
	std::vector<glm::vec3> roadStreetTris;
	std::vector<glm::vec3> sidewalkTris;

	//sidewalk variables
	road::LineRenderer sidewalkLines;
	std::vector<glm::vec3> sidewalkLineVerts;

};

struct LotContext
{

	//lot variables
	road::LotSubdivision lotGen;
	road::LotCollection lots;
	road::LineRenderer lotLines;
	std::vector<glm::vec3> lotLineVerts;

	//garden variables
	road::LineRenderer gardenLines;
	std::vector<glm::vec3> gardenLineVerts;

	//building footprint variables
	road::LineRenderer footprintLines;
	std::vector<glm::vec3> footprintLineVerts;

};

struct TextureContext
{

	//building window texture variables
	road::BuildingTexturedRenderer windowMeshTex;

	//roof texture variables
	road::BuildingTexturedRenderer roadMeshHighwayTex;
	road::BuildingTexturedRenderer roadMeshStreetTex;
	road::BuildingTexturedRenderer sidewalkMeshTex;
	road::BuildingTexturedRenderer groundMeshTex;

	std::vector<road::BuildingVertexPT> roofUrbanPT;
	std::vector<road::BuildingVertexPT> roofSuburbanPT;
	std::vector<road::BuildingVertexPT> roofRuralPT;
	std::vector<road::BuildingVertexPT> roadHighwayPT;
	std::vector<road::BuildingVertexPT> roadStreetPT;
	std::vector<road::BuildingVertexPT> sidewalkPT;
	std::vector<road::BuildingVertexPT> groundPT;

	//roof meshes by type
	road::BuildingTexturedRenderer roofMeshUrban;
	road::BuildingTexturedRenderer roofMeshSuburban;
	road::BuildingTexturedRenderer roofMeshRural;

	//building texture variables
	road::BuildingTexturedRenderer buildingMeshUrban;
	road::BuildingTexturedRenderer buildingMeshSuburban;
	road::BuildingTexturedRenderer buildingMeshRural;

	std::vector<road::BuildingVertexPT> buildingUrbanPT;
	std::vector<road::BuildingVertexPT> buildingSuburbanPT;
	std::vector<road::BuildingVertexPT> buildingRuralPT;

	//window texture
	GLuint windowTex = 0;
	GLuint roadTex = 0;
	GLuint sidewalkTex = 0;
	GLuint groundTex = 0;

	bool useWindowTex = false;
	bool useRoadTex = false;
	bool useSidewalkTex = false;
	bool useGroundTex = false;

	GLuint urbanTex = 0, suburbanTex = 0, ruralTex = 0;
	bool useUrban = false, useSuburban = false, useRural = false;

	GLuint roofUrbanTex = 0, roofSuburbanTex = 0, roofRuralTex = 0;
	bool useRoofUrban = false, useRoofSuburban = false, useRoofRural = false;

};

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


	//draws a simple modal-style overlay while city generation is in progress
	static void DrawGeneratingOverlay()
	{
		if (!gShowGeneratingOverlay) return;

		ImGuiViewport* vp = ImGui::GetMainViewport();
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


	//prints the current road-generation parameters for debugging
	void PrintRoadParams(const road::RoadParams& rp)
	{
		std::cout
		<< "[GUI] cityRadius=" << rp.cityRadius
		<< " maxIterations=" << rp.maxIterations
		<< " maxStreets=" << rp.maxStreetSegments   
		<< " maxHighways=" << rp.maxHighwaySegments 
		<< "\n";
	}

	//converts plain triangle vertices into textured vertices using world-space XZ UV mapping
	void ConvertTriVertsToPT_WorldXZ(const std::vector<glm::vec3>& in,
                                    std::vector<road::BuildingVertexPT>& out,
                                    float uvMetersPerTile)
    {
        out.clear();
        out.reserve(in.size());

        if (uvMetersPerTile <= 0.001f) uvMetersPerTile = 2.0f;
        const float s = 1.0f / uvMetersPerTile;

        for (const auto& p : in)
        {
            road::BuildingVertexPT v;
            v.pos = p;
            v.uv = glm::vec2(p.x, p.z) * s;
            out.push_back(v);
        }
    }

	//builds a textured ground quad as two triangles
    void BuildGroundPT(std::vector<road::BuildingVertexPT>& out,
                       const glm::vec2& centerXZ,
                       float halfW,
                       float halfH,
                       float y,
                       float uvMetersPerTile)
    {
        out.clear();
        if (uvMetersPerTile <= 0.001f) uvMetersPerTile = 4.0f;
        const float s = 1.0f / uvMetersPerTile;

        glm::vec3 a(centerXZ.x - halfW, y, centerXZ.y - halfH);
        glm::vec3 b(centerXZ.x + halfW, y, centerXZ.y - halfH);
        glm::vec3 c(centerXZ.x + halfW, y, centerXZ.y + halfH);
        glm::vec3 d(centerXZ.x - halfW, y, centerXZ.y + halfH);

        auto UV = [&](const glm::vec3& p){ return glm::vec2(p.x, p.z) * s; };

        // two tris: a-b-c, a-c-d
        out.push_back({a, UV(a)});
        out.push_back({b, UV(b)});
        out.push_back({c, UV(c)});

        out.push_back({a, UV(a)});
        out.push_back({c, UV(c)});
        out.push_back({d, UV(d)});
    }

	//build road and sidewalk surface meshes then uploads them to GPU renderers
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

	//builds and uploads debug line geometry for highways and streets
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
	//derives lot generation parameters from the current GUI and road settings
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
	//prints lot zoning and garden-generation statistics for debugging
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

	//builds and uploads all lot-derived geometry:
	//lots, sidewalks, gardens, footprints, buildings,roofs and windows
	void BuildAndUploadLotDerivedGeometry(
		const road::RoadNetwork& roadNet,
		const road::RoadParams& roadParams,
		const road::LotCollection& lots,
		std::vector<glm::vec3>& lotLineVerts,
		std::vector<glm::vec3>& sidewalkLineVerts,
		std::vector<glm::vec3>& gardenLineVerts,
		std::vector<glm::vec3>& footprintLineVerts,
		std::vector<glm::vec3>& buildingTriVerts,
		std::vector<glm::vec3>& buildingQuadVerts,
		std::vector<road::BuildingVertexPT>& buildingUrbanPT,
		std::vector<road::BuildingVertexPT>& buildingSuburbanPT,
		std::vector<road::BuildingVertexPT>& buildingRuralPT,
		road::LineRenderer& lotLines,
		road::LineRenderer& sidewalkLines,
		road::LineRenderer& gardenLines,
		road::LineRenderer& footprintLines,
		road::BuildingRenderer& buildingMesh,
		road::BuildingRenderer& roofMesh,
		road::BuildingRenderer& windowMesh,
		road::BuildingTexturedRenderer& buildingMeshUrban,
		road::BuildingTexturedRenderer& buildingMeshSuburban,
		road::BuildingTexturedRenderer& buildingMeshRural,
		std::vector<glm::vec3>& buildingRoofQuadVerts,
		std::vector<glm::vec3>& buildingRoofTriVerts,
		std::vector<glm::vec3>& buildingWindowQuadVerts,
		std::vector<glm::vec3>& buildingWindowTriVerts)
	{

		const float baseY = 0.03f;
		const float floorH = 0.35f;

		//build debug line geometry for lot-related overlays
		road::BuildLotLineVerts(lots, lotLineVerts, 0.02f);
		road::BuildSidewalkLineVerts(roadNet, roadParams, sidewalkLineVerts, 0.06f);
		road::BuildGardenLineVerts(lots, gardenLineVerts, 0.021f);
		road::BuildFootprintLineVerts(lots, footprintLineVerts, 0.022f);

		//triangles for rendering
		road::BuildBuildingTriVerts(lots, buildingTriVerts, baseY, floorH);

		//quads for exporting
		road::BuildBuildingQuadVerts(lots, buildingQuadVerts, baseY, floorH);

		road::BuildBuildingTriVertsTexturedByZone(lots, buildingUrbanPT, buildingSuburbanPT, buildingRuralPT,
													baseY, floorH, 2.0f);


		// roof details
		road::BuildRoofDetailVerts(lots, buildingRoofQuadVerts, buildingRoofTriVerts, baseY, floorH);

		road::BuildRoofDetailTriVertsTexturedByZone(
			lots,
			sTex.roofUrbanPT, sTex.roofSuburbanPT, sTex.roofRuralPT,
			baseY, floorH,
			2.0f
		);

		sTex.roofMeshUrban.Upload(sTex.roofUrbanPT);
		sTex.roofMeshSuburban.Upload(sTex.roofSuburbanPT);
		sTex.roofMeshRural.Upload(sTex.roofRuralPT);



		// windows (quads)
		road::BuildWindowDetailQuads(lots, buildingWindowQuadVerts, baseY, floorH);

		// convert window quads -> tris for rendering with BuildingRenderer
		buildingWindowTriVerts.clear();
		buildingWindowTriVerts.reserve((buildingWindowQuadVerts.size() / 4) * 6);
		for (size_t i = 0; i + 3 < buildingWindowQuadVerts.size(); i += 4)
		{
			const auto& a = buildingWindowQuadVerts[i+0];
			const auto& b = buildingWindowQuadVerts[i+1];
			const auto& c = buildingWindowQuadVerts[i+2];
			const auto& d = buildingWindowQuadVerts[i+3];
			// two tris
			buildingWindowTriVerts.push_back(a);
			buildingWindowTriVerts.push_back(b);
			buildingWindowTriVerts.push_back(c);
			buildingWindowTriVerts.push_back(a);
			buildingWindowTriVerts.push_back(c);
			buildingWindowTriVerts.push_back(d);
		}

		std::vector<road::BuildingVertexPT> windowPTTris;
		windowPTTris.reserve((buildingWindowQuadVerts.size() / 4) * 6);

		for (size_t i = 0; i + 3 < buildingWindowQuadVerts.size(); i += 4)
		{
			const glm::vec3& a = buildingWindowQuadVerts[i + 0];
			const glm::vec3& b = buildingWindowQuadVerts[i + 1];
			const glm::vec3& c = buildingWindowQuadVerts[i + 2];
			const glm::vec3& d = buildingWindowQuadVerts[i + 3];

			// UVs (simple quad mapping)
			road::BuildingVertexPT A{a, glm::vec2(0.0f, 0.0f)};
			road::BuildingVertexPT B{b, glm::vec2(1.0f, 0.0f)};
			road::BuildingVertexPT C{c, glm::vec2(1.0f, 1.0f)};
			road::BuildingVertexPT D{d, glm::vec2(0.0f, 1.0f)};

			// two tris
			windowPTTris.push_back(A);
			windowPTTris.push_back(B);
			windowPTTris.push_back(C);

			windowPTTris.push_back(A);
			windowPTTris.push_back(C);
			windowPTTris.push_back(D);
		}

		std::vector<glm::vec3> roofRenderTris;
		roofRenderTris.reserve((buildingRoofQuadVerts.size() / 4) * 6 + buildingRoofTriVerts.size());

		// convert roof quads -> tris
		for (size_t i = 0; i + 3 < buildingRoofQuadVerts.size(); i += 4)
		{
			const glm::vec3& a = buildingRoofQuadVerts[i + 0];
			const glm::vec3& b = buildingRoofQuadVerts[i + 1];
			const glm::vec3& c = buildingRoofQuadVerts[i + 2];
			const glm::vec3& d = buildingRoofQuadVerts[i + 3];

			// two triangles: a-b-c and a-c-d
			roofRenderTris.push_back(a);
			roofRenderTris.push_back(b);
			roofRenderTris.push_back(c);

			roofRenderTris.push_back(a);
			roofRenderTris.push_back(c);
			roofRenderTris.push_back(d);
		}

		// append any roof tris (rural gable end caps )
		roofRenderTris.insert(roofRenderTris.end(), buildingRoofTriVerts.begin(), buildingRoofTriVerts.end());

		std::cout << "Building tri verts: " << buildingTriVerts.size() << "\n";
		std::cout << "Building quad verts: " << buildingQuadVerts.size() << "\n";
		lotLines.Upload(lotLineVerts);
		sidewalkLines.Upload(sidewalkLineVerts);
		gardenLines.Upload(gardenLineVerts);
		footprintLines.Upload(footprintLineVerts);
		buildingMesh.Upload(buildingTriVerts);
		buildingMeshUrban.Upload(buildingUrbanPT);
		buildingMeshSuburban.Upload(buildingSuburbanPT);
		buildingMeshRural.Upload(buildingRuralPT);
		roofMesh.Upload(roofRenderTris);
		windowMesh.Upload(buildingWindowTriVerts);
		sTex.windowMeshTex.Upload(windowPTTris);



		std::cout << "Lots count: " << lots.lots.size() << "\n";
		std::cout << "Lot verts: " << lotLineVerts.size() << "\n";
		std::cout << "Garden verts: " << gardenLineVerts.size() << "\n";
	}

	//updates the OpenGL viewport and clears the frame buffers
	void viewPort_Setup(GLFWwindow *window)
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		glClearColor(0.40f, 0.52f, 0.43f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	}

	//handles basic window input such as closing on escape
	void processInput(GLFWwindow *window)
	{
		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
	//GLFW error callback
	void error_callback(int error, const char *description)
	{
		std::cout << "Error: " <<  description << "\n";
	}

	//captures mouse wheel input for 2D zoom control
	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		ImGuiIO& io = ImGui::GetIO();


		if (io.WantCaptureMouse)
			return;


		gScrollY += (float)yoffset;
	}

	//loads a 2D texture from disk and creates an OpenGL texture object
	GLuint LoadTexture2D(const std::string& path)
	{
		int w=0,h=0,channels=0;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
		if (!data) return 0;

		GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

		GLuint tex=0;
		glGenTextures(1,&tex);
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

	//deletes a texture safely and resets its handle
	void SafeDeleteTexture(GLuint& tex)
	{
		if (tex != 0)
		{
			glDeleteTextures(1, &tex);
			tex = 0;
		}
	}
	//handles GUI requests for applying or clearing the window texture
	void HandleWindowTextureRequests(Gui& gui, GLuint& windowTex, bool& useWindowTex)
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
	//handles GUI requests for road, sidewlak and ground textures
	void HandleSceneTextureRequests(Gui& gui,
                                    GLuint& roadTex, bool& useRoad,
                                    GLuint& sidewalkTex, bool& useSidewalk,
                                    GLuint& groundTex, bool& useGround)
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

	//handles GUI requests for urban, suburban and rural building textures
	void HandleBuildingTextureRequests(
		Gui& gui,
		GLuint& urbanTex, GLuint& suburbanTex, GLuint& ruralTex,
		bool& useUrban, bool& useSuburban, bool& useRural)
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

	//handles GUI requests for urban, suburban and rural roof textures
	void HandleRoofTextureRequests(
		Gui& gui,
		GLuint& urbanRoof, GLuint& suburbanRoof, GLuint& ruralRoof,
		bool& useUrbanRoof, bool& useSuburbanRoof, bool& useRuralRoof)
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


	//resolves the active city centre and radius for rendering camera logic
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

	//applies GUI-driven camera mode changes and resets requests
	bool UpdateCameraModeAndResetFromGui(Gui& gui, Camera& camera, const CityContext& cityCTX) {

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

	//computes frame timing, frambuffer size and the current view projection matrix
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

	//handles per-frame paltform input and viewport setup
	void HandlePlatformInputAndViewport(GLFWwindow* window) {

		processInput(window);
		viewPort_Setup(window);
	}

	//updates free-fly 3D camera controls using mouse + keyboard input
	void UpdateCamera3DControls(Camera& camera, GLFWwindow* window, float deltaTime) {
			
			bool rmbDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

			double mx = 0.0, my =0.0;
			glfwGetCursorPos(window, &mx, &my);

			camera.OnMouseMove(mx, my, rmbDown);

			glfwSetInputMode(window, GLFW_CURSOR, rmbDown ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

			camera.UpdateFly3D(window, deltaTime);
	}

	//updates 2D pan and zoom camera controls
	void UpdateCamera2DControls(Camera& camera, GLFWwindow* window, float deltaTime) {
			
			camera.UpdatePanXZ(window, deltaTime);
			camera.ApplyScrollZoom(gScrollY);
			gScrollY = 0.0f;
	}
	
	//routes camera updates to either 2D or 3D controls
	void UpdateCameraPerFrame(Camera& camera, GLFWwindow* window, float deltaTime, bool is3D) {
		
		if (is3D) {
			UpdateCamera3DControls(camera, window, deltaTime);
		} else {
			UpdateCamera2DControls(camera, window, deltaTime);
		}
	}

	//draws the 3D city meshes, including roads, sidewalks, buildings, roofs and windows
	void Draw3DMeshesIfEnabled(
		Gui& gui, bool is3D,
		Shader& lineShader,
		Shader& litShader,
		const glm::mat4& viewProjection,

		road::BuildingRenderer& roadMeshStreet,
		road::BuildingRenderer& roadMeshHighway,
		road::BuildingRenderer& sidewalkMesh,
		road::BuildingRenderer& buildingMesh,

		road::BuildingTexturedRenderer& buildingMeshUrban,
		road::BuildingTexturedRenderer& buildingMeshSuburban,
		road::BuildingTexturedRenderer& buildingMeshRural,

		road::BuildingTexturedRenderer& roofMeshUrban,
		road::BuildingTexturedRenderer& roofMeshSuburban,
		road::BuildingTexturedRenderer& roofMeshRural,

		road::BuildingTexturedRenderer& windowMeshTex,

		road::BuildingTexturedRenderer& roadMeshStreetTex,
		road::BuildingTexturedRenderer& roadMeshHighwayTex,
		road::BuildingTexturedRenderer& sidewalkMeshTex,

		GLuint urbanTex, GLuint suburbanTex, GLuint ruralTex,
		GLuint roofUrbanTex, GLuint roofSuburbanTex, GLuint roofRuralTex,
		GLuint roadTex, GLuint sidewalkTex,
		GLuint windowTex,

		bool useUrban, bool useSuburban, bool useRural,
		bool useRoofUrban, bool useRoofSuburban, bool useRoofRural,
		bool useRoadTex, bool useSidewalkTex,
		bool useWindowTex
	)
	{
		if (!is3D) return;

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		if (useRoadTex && roadTex != 0)
		{
			const glm::vec3 roadFallback(0.10f, 0.10f, 0.10f);
			roadMeshStreetTex.Draw(litShader, viewProjection, roadTex, true, roadFallback);
			roadMeshHighwayTex.Draw(litShader, viewProjection, roadTex, true, roadFallback);
		}
		else
		{
			litShader.use();
			glUniform1i(glGetUniformLocation(litShader.ID, "uUseTexture"), 0);

			glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.12f, 0.12f, 0.12f);
			roadMeshStreet.Draw(litShader, viewProjection);

			glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.07f, 0.07f, 0.07f);
			roadMeshHighway.Draw(litShader, viewProjection);
		}

		if (useSidewalkTex && sidewalkTex != 0)
		{
			const glm::vec3 swFallback(0.70f, 0.70f, 0.70f);
			sidewalkMeshTex.Draw(litShader, viewProjection, sidewalkTex, true, swFallback);
		}
		else
		{
			litShader.use();
			glUniform1i(glGetUniformLocation(litShader.ID, "uUseTexture"), 0);

			glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.70f, 0.70f, 0.70f);
			sidewalkMesh.Draw(litShader, viewProjection);
		}

		const bool anyTextured = (useUrban || useSuburban || useRural);

		if (anyTextured)
		{
			const glm::vec3 urbanFallback(0.78f, 0.78f, 0.80f);
			const glm::vec3 subFallback  (0.75f, 0.75f, 0.78f);
			const glm::vec3 rurFallback  (0.72f, 0.72f, 0.75f);

			buildingMeshUrban.Draw(litShader, viewProjection, urbanTex,    useUrban,    urbanFallback);
			buildingMeshSuburban.Draw(litShader, viewProjection, suburbanTex, useSuburban, subFallback);
			buildingMeshRural.Draw(litShader, viewProjection, ruralTex,    useRural,    rurFallback);
		}
		else
		{
			litShader.use();
			glUniform1i(glGetUniformLocation(litShader.ID, "uUseTexture"), 0);
			glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.75f, 0.75f, 0.78f);
			buildingMesh.Draw(litShader, viewProjection);
		}

		// roofs
		if (gui.RenderBuildingRoofs())
		{
			glDisable(GL_CULL_FACE);

			glEnable(GL_POLYGON_OFFSET_FILL);
			//to avoid Z-fighting
			glPolygonOffset(-1.0f, -1.0f);

			const glm::vec3 roofFallback(0.65f, 0.65f, 0.67f);
			roofMeshUrban.Draw(litShader, viewProjection, roofUrbanTex, useRoofUrban, roofFallback);
			roofMeshSuburban.Draw(litShader, viewProjection, roofSuburbanTex, useRoofSuburban, roofFallback);
			roofMeshRural.Draw(litShader, viewProjection, roofRuralTex, useRoofRural, roofFallback);

			glDisable(GL_POLYGON_OFFSET_FILL);

			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}



		// windows
		if (gui.RenderBuildingWindows())
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1.0f, -1.0f);

			glDepthMask(GL_FALSE);

			windowMeshTex.Draw(
				litShader,
				viewProjection,
				windowTex,
				useWindowTex,
				glm::vec3(0.20f, 0.35f, 0.55f)
			);

			glDisable(GL_POLYGON_OFFSET_FILL);

			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}

	}

	//draws he ground plane and returns its extents for export use
	GroundContext DrawGroundAndGetExtents(
		Primitives& primitives,
		Shader& primShader,
		Shader& litShader,
		const glm::mat4& viewProjection,
		const CityContext& cityCTX,
		bool useGroundTex,
		GLuint groundTex,
		road::BuildingTexturedRenderer& groundMeshTex,
		std::vector<road::BuildingVertexPT>& groundPT)
	{
		GroundContext g;
		g.margin = 20.0f;

		// ground half extents based on city size
		g.halfW = cityCTX.cityR + g.margin;
		g.halfH = cityCTX.cityR + g.margin;

		if (useGroundTex && groundTex != 0)
		{
			BuildGroundPT(
				groundPT,
				cityCTX.centerXZ,
				g.halfW,
				g.halfH,
				0.0f,      // y
				6.0f       // uv meters per tile 
			);

			groundMeshTex.Upload(groundPT);

			groundMeshTex.Draw(litShader, viewProjection, groundTex, groundTex != 0, glm::vec3(0.0f, 0.30f, 0.0f));
		}
		else
		{
			primitives.DrawGround(primShader, viewProjection, cityCTX.centerXZ, g.halfW, g.halfH);
		}

		return g;
	}

	static std::vector<glm::vec3> StripBuildingTopCapQuads(const std::vector<glm::vec3>& buildingQuadVerts)
	{

		std::vector<glm::vec3> out;
		out.reserve(buildingQuadVerts.size()); 

		const size_t vertsPerBuilding = 5 * 4; // 20
		const size_t keepVerts = 4 * 4;        // 16

		for (size_t i = 0; i + vertsPerBuilding <= buildingQuadVerts.size(); i += vertsPerBuilding)
		{
			// copy only wall quads
			out.insert(out.end(),
					buildingQuadVerts.begin() + i,
					buildingQuadVerts.begin() + i + keepVerts);
		}

		return out;
	}

	//exports the generated city geometry if requested through th eGUI
	void HandleExportIfRequested(Gui& gui, const CityContext& cityCTX, const GroundContext& ground,
										const std::vector<glm::vec3>& roadHighwayTris,
										const std::vector<glm::vec3>& roadStreetTris,
										const std::vector<glm::vec3>& sidewalkTris,
										const std::vector<glm::vec3>& buildingTriVerts,
										const std::vector<glm::vec3>& buildingQuadVerts,
										const std::vector<glm::vec3>& buildingRoofQuadVerts,
										const std::vector<glm::vec3>& buildingRoofTriVerts,
										const std::vector<glm::vec3>& buildingWindowQuadVerts,
										const std::vector<glm::vec3>& buildingWindowTriVerts)
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
				ex.buildingRoofTris  = buildingRoofTriVerts;
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

	//draws optional debug overlays such as roads, lots, gardens and footprints
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
		
		if (gui.ShowRoadLines())
		{
			//highways
			glLineWidth(4.0f);
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
			highwayLines.Draw(lineShader, viewProjection);

			//streets
			glLineWidth(1.0f);
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
			streetLines.Draw(lineShader, viewProjection);
		}
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

	//closes the application if the GUI requested a quit action
	void HandleQuitIfRequested(Gui& gui, GLFWwindow* window)
	{
		if (gui.WantsQuit())
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	//finalises and renders the ImGui frame
	void EndGuiFrame(Gui& gui)
	{
		gui.EndFrameGUI();

	}

	//begins a new ImGui frame and draws the GUI panels
	void BeginGuiFrame(Gui& gui)
	{
		gui.BeginFrameGUI();
		gui.DrawGUI();
	}

	//presents the rendered frame and processes window events 
	void PresentAndPoll(GLFWwindow* window)
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//runs the full city generation pipeline and uploads all resulting geometry
	void GenerateCityNow(
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
		road::BuildingTexturedRenderer& buildingMeshUrban,
		road::BuildingTexturedRenderer& buildingMeshSuburban,
		road::BuildingTexturedRenderer& buildingMeshRural,
		road::BuildingTexturedRenderer& roadMeshHighwayTex,
		road::BuildingTexturedRenderer& roadMeshStreetTex,
		road::BuildingTexturedRenderer& sidewalkMeshTex,

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
		std::vector<glm::vec3>& buildingTriVerts,
		std::vector<glm::vec3>& buildingQuadVerts,

		// textures:
		std::vector<road::BuildingVertexPT>& buildingUrbanPT,
		std::vector<road::BuildingVertexPT>& buildingSubUrbanPT,
		std::vector<road::BuildingVertexPT>& buildingRuralPT,
		std::vector<road::BuildingVertexPT>& roadHighwayPT,
		std::vector<road::BuildingVertexPT>& roadStreetPT,
		std::vector<road::BuildingVertexPT>& sidewalkPT)
	{

		// pull params from GUI
		roadParams = gui.GetParams();
		PrintRoadParams(roadParams);

		//start timer
		using Clock = std::chrono::high_resolution_clock;
		auto t0 = Clock::now();

		// generate road network
		roadNet = roadGen.Generate(roadParams);

		// build & upload road + sidewalk slabs
		BuildAndUploadRoadAndSidewalkMeshes(
			roadNet, roadParams,
			roadHighwayTris, roadStreetTris, sidewalkTris,
			roadMeshHighway, roadMeshStreet, sidewalkMesh);

		// build PT buffers (UVs) + upload textured renderers
		ConvertTriVertsToPT_WorldXZ(roadHighwayTris, roadHighwayPT, 3.0f);
		ConvertTriVertsToPT_WorldXZ(roadStreetTris,  roadStreetPT,  3.0f);
		ConvertTriVertsToPT_WorldXZ(sidewalkTris,    sidewalkPT,    2.0f);

		roadMeshHighwayTex.Upload(roadHighwayPT);
		roadMeshStreetTex.Upload(roadStreetPT);
		sidewalkMeshTex.Upload(sidewalkPT);

		// build & upload highway/street line renderers
		showRoads = true;
		BuildAndUploadRoadLineVerts(roadNet, highwayLines, streetLines);

		// lot generation
		road::LotParams lotParams = MakeLotParamsFromRoadParams(gui, roadParams);
		lots = lotGen.GenerateLots(roadNet, lotParams);

		// debug stats
		PrintGardenPipelineStats(lots, lotParams);

		// build derived geometry (lots/sidewalk/gardens/footprints/buildings) + upload
		BuildAndUploadLotDerivedGeometry(
			roadNet, roadParams, lots,
			lotLineVerts, sidewalkLineVerts, gardenLineVerts, footprintLineVerts,
			buildingTriVerts, buildingQuadVerts, buildingUrbanPT, buildingSubUrbanPT,
			buildingRuralPT, lotLines, sidewalkLines, gardenLines, footprintLines,
			buildingMesh, sBuilding.roofMesh, sBuilding.windowMesh,
			buildingMeshUrban, buildingMeshSuburban, buildingMeshRural,
			sBuilding.buildingRoofQuadVerts, sBuilding.buildingRoofTriVerts, sBuilding.buildingWindowQuadVerts,
			sBuilding.buildingWindowTriVerts);


		auto t1 = Clock::now();
		gLastGenerationMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		gui.SetLastGenerationMs(gLastGenerationMs);
	}



	//triggers city generation when requested by the GUI
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
			road::BuildingTexturedRenderer& buildingMeshUrban,
			road::BuildingTexturedRenderer& buildingMeshSuburban,
			road::BuildingTexturedRenderer& buildingMeshRural,
			road::BuildingTexturedRenderer& roadMeshHighwayTex,
			road::BuildingTexturedRenderer& roadMeshStreetTex,
			road::BuildingTexturedRenderer& sidewalkMeshTex,


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
			std::vector<glm::vec3>& buildingTriVerts,
			std::vector<glm::vec3>& buildingQuadVerts,

			//textures:
			std::vector<road::BuildingVertexPT>& buildingUrbanPT,
			std::vector<road::BuildingVertexPT>& buildingSubUrbanPT,
			std::vector<road::BuildingVertexPT>& buildingRuralPT,
			std::vector<road::BuildingVertexPT>& roadHighwayPT,
			std::vector<road::BuildingVertexPT>& roadStreetPT,
			std::vector<road::BuildingVertexPT>& sidewalkPT)
	{
			if (!gui.WantsGenerate())
			return;

		GenerateCityNow(
			gui, showRoads, roadGen, lotGen, roadParams, roadNet, lots,
			roadMeshHighway, roadMeshStreet, sidewalkMesh, buildingMesh,
			buildingMeshUrban, buildingMeshSuburban, buildingMeshRural,
			roadMeshHighwayTex, roadMeshStreetTex, sidewalkMeshTex,
			highwayLines, streetLines, lotLines, sidewalkLines, gardenLines, footprintLines,
			roadHighwayTris, roadStreetTris, sidewalkTris,
			lotLineVerts, sidewalkLineVerts, gardenLineVerts, footprintLineVerts,
			buildingTriVerts, buildingQuadVerts,
			buildingUrbanPT, buildingSubUrbanPT, buildingRuralPT,
			roadHighwayPT, roadStreetPT, sidewalkPT
		);


			

			// pull params from GUI
			roadParams = gui.GetParams();
			PrintRoadParams(roadParams);

			//start timer
			using Clock = std::chrono::high_resolution_clock;
			auto t0 = Clock::now();

			// generate road network
			roadNet = roadGen.Generate(roadParams);

			// build & upload road + sidewalk slabs
			BuildAndUploadRoadAndSidewalkMeshes(
				roadNet, roadParams,
				roadHighwayTris, roadStreetTris, sidewalkTris,
				roadMeshHighway, roadMeshStreet, sidewalkMesh);

			// build PT buffers (UVs) + upload textured renderers
			ConvertTriVertsToPT_WorldXZ(roadHighwayTris, roadHighwayPT, 3.0f);
			ConvertTriVertsToPT_WorldXZ(roadStreetTris,  roadStreetPT,  3.0f);
			ConvertTriVertsToPT_WorldXZ(sidewalkTris,    sidewalkPT,    2.0f);

			roadMeshHighwayTex.Upload(roadHighwayPT);
			roadMeshStreetTex.Upload(roadStreetPT);
			sidewalkMeshTex.Upload(sidewalkPT);


			// build & upload highway/street line renderers
			showRoads = true;
			BuildAndUploadRoadLineVerts(roadNet, highwayLines, streetLines);

			// lot generation
			road::LotParams lotParams = MakeLotParamsFromRoadParams(gui, roadParams);
			lots = lotGen.GenerateLots(roadNet, lotParams);

			// debug stats
			PrintGardenPipelineStats(lots, lotParams);

			// build derived geometry (lots/sidewalk/gardens/footprints/buildings) + upload
			BuildAndUploadLotDerivedGeometry(
				roadNet, roadParams, lots,
				lotLineVerts, sidewalkLineVerts, gardenLineVerts, footprintLineVerts,
				buildingTriVerts, buildingQuadVerts, buildingUrbanPT, buildingSubUrbanPT,
				buildingRuralPT, lotLines, sidewalkLines, gardenLines, footprintLines,
				buildingMesh, sBuilding.roofMesh, sBuilding.windowMesh,
				buildingMeshUrban, buildingMeshSuburban, buildingMeshRural,
				sBuilding.buildingRoofQuadVerts, sBuilding.buildingRoofTriVerts, sBuilding.buildingWindowQuadVerts,
				sBuilding.buildingWindowTriVerts);

			auto t1 = Clock::now();
			gLastGenerationMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
			gui.SetLastGenerationMs(gLastGenerationMs);
	}

	//computes the directional light vector and light-space matrix for shadow mapping
	static void ComputeLightSpace(
		const CityContext& city,
		glm::vec3& outLightDir,
		glm::mat4& outLightSpace)
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

	//renders the depth-only shadow pass from the light's perspective
	static void RenderShadowPassIf3D(
		bool is3D,
		Gui& gui,
		ShadowMap& shadowMap,
		Shader& shadowDepthShader,
		const glm::mat4& lightSpace,
		const FrameContext& frame,

		// toggles + textures
		bool useUrban, bool useSuburban, bool useRural,
		bool useRoadTex, unsigned int roadTex,
		bool useSidewalkTex, unsigned int sidewalkTex,

		// non-textured renderers
		road::BuildingRenderer& roadMeshHighway,
		road::BuildingRenderer& roadMeshStreet,
		road::BuildingRenderer& sidewalkMesh,
		road::BuildingRenderer& buildingMesh,

		// textured renderers
		road::BuildingTexturedRenderer& roadMeshStreetTex,
		road::BuildingTexturedRenderer& roadMeshHighwayTex,
		road::BuildingTexturedRenderer& sidewalkMeshTex,

		road::BuildingTexturedRenderer& buildingMeshUrban,
		road::BuildingTexturedRenderer& buildingMeshSuburban,
		road::BuildingTexturedRenderer& buildingMeshRural,

		road::BuildingTexturedRenderer& roofMeshUrban,
		road::BuildingTexturedRenderer& roofMeshSuburban,
		road::BuildingTexturedRenderer& roofMeshRural,

		road::BuildingTexturedRenderer& windowMeshTex,
		road::BuildingTexturedRenderer& groundMeshTex)
	{
		if (!is3D) return;

		shadowMap.BeginDepthPass();

		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(2.5f, 4.0f);

		shadowDepthShader.use();

		const bool anyBuildingTex = (useUrban || useSuburban || useRural);

		// roads
		if (useRoadTex && roadTex != 0)
		{
			roadMeshStreetTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
			roadMeshHighwayTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
		}
		else
		{
			roadMeshHighway.Draw(shadowDepthShader, lightSpace);
			roadMeshStreet.Draw(shadowDepthShader, lightSpace);
		}

		// sidewalks
		if (useSidewalkTex && sidewalkTex != 0)
		{
			sidewalkMeshTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
		}
		else
		{
			sidewalkMesh.Draw(shadowDepthShader, lightSpace);
		}

		// building bases
		if (anyBuildingTex)
		{
			buildingMeshUrban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
			buildingMeshSuburban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
			buildingMeshRural.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
		}
		else
		{
			buildingMesh.Draw(shadowDepthShader, lightSpace);
		}

		// roofs
		if (gui.RenderBuildingRoofs())
		{
			roofMeshUrban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
			roofMeshSuburban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
			roofMeshRural.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
		}

		// windows
		if (gui.RenderBuildingWindows())
		{
			windowMeshTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
		}

		// ground
		groundMeshTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));

		glDisable(GL_POLYGON_OFFSET_FILL);

		shadowMap.EndDepthPass(frame.w, frame.h);
	}

	//uploads lighting and shadow-map uniforms for the main lit shader
	static void SetupLightingAndShadowUniforms(
		Shader& litShader,
		ShadowMap& shadowMap,
		const glm::mat4& lightSpace,
		const glm::vec3& lightDir,
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

	//draws the skybox as the background environment
	static void DrawSkyboxPass(
		Skybox& skybox,
		Camera& camera,
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

	//queues delayed generation and updates the loading overlay state
	static void HandleGenerationOverlayAndQueue(Gui& gui)
	{
		//show overlay first and delay generation to next frame
		if (gui.WantsGenerate())
		{
			gShowGeneratingOverlay = true;
			gGeneratePending = true;
		}

		// draw loading overlay on top of everything
		DrawGeneratingOverlay();
	}

}

//initialises the widnow, openGL, GUI, renderers and the main frame loop
int main(void)
{

	glfwSetErrorCallback(error_callback);

	//initialize
	if (!glfwInit())
		exit(EXIT_FAILURE);

	//request OpenGL 3.3 context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

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

	//primitives initialization
	sInit.primitives.Initialize_Prim();

	//camera initialization
	sInit.camera.Set3DEnabled(false, glm::vec2(0.0f, 0.0f), gui.GetCityRadius());
	

	//shader paths
	Shader primShader("../include/Shaders/basic.vert", "../include/Shaders/basic.frag");
	Shader lineShader("../include/Shaders/line.vert", "../include/Shaders/line.frag");
	Shader buildingTexShader("../include/Shaders/building_tex.vert", "../include/Shaders/building_tex.frag");
	Shader shadowDepthShader("../include/Shaders/shadow_depth.vert", "../include/Shaders/shadow_depth.frag");
	Shader litShader("../include/Shaders/lit_shadow.vert", "../include/Shaders/lit_shadow.frag");

	//skybox initilization
	if (!sInit.skybox.Initialize("../assets/textures/SkyBox/Standard-Cube-Map"))
	{
		std::cout << "Skybox init failed. \n";
	}

	//initialization block of all 
	auto Fail = [](const char* what)
	{
		std::cout << "Init failed: " << what << "\n";
		return -1;
	};

	// debug line renderers
	if (!sRoads.highwayLines.Initialize_Road())   return Fail("highwayLines");
	if (!sRoads.streetLines.Initialize_Road())    return Fail("streetLines");
	if (!sLots.lotLines.Initialize_Road())       return Fail("lotLines");
	if (!sRoads.sidewalkLines.Initialize_Road())  return Fail("sidewalkLines");
	if (!sLots.gardenLines.Initialize_Road())    return Fail("gardenLines");
	if (!sLots.footprintLines.Initialize_Road()) return Fail("footprintLines");

	// non-textured meshes
	if (!sBuilding.buildingMesh.Initialize())        return Fail("buildingMesh");
	if (!sRoads.roadMeshHighway.Initialize())     return Fail("roadMeshHighway");
	if (!sRoads.roadMeshStreet.Initialize())      return Fail("roadMeshStreet");
	if (!sRoads.sidewalkMesh.Initialize())        return Fail("sidewalkMesh");

	// textured building meshes (urban/suburban/rural)
	if (!sTex.buildingMeshUrban.Initialize())   return Fail("buildingMeshUrban");
	if (!sTex.buildingMeshSuburban.Initialize())return Fail("buildingMeshSuburban");
	if (!sTex.buildingMeshRural.Initialize())   return Fail("buildingMeshRural");

	// details
	if (!sBuilding.roofMesh.Initialize())            return Fail("roofMesh");
	if (!sBuilding.windowMesh.Initialize())          return Fail("windowMesh");

	// textured windows (alpha windows)
	if (!sTex.windowMeshTex.Initialize())       return Fail("windowMeshTex");

	// textured roofs
	if (!sTex.roofMeshUrban.Initialize())    return Fail("roofMeshUrban");
	if (!sTex.roofMeshSuburban.Initialize()) return Fail("roofMeshSuburban");
	if (!sTex.roofMeshRural.Initialize())    return Fail("roofMeshRural");

	// road, sidewalk and ground texture
	if (!sTex.roadMeshHighwayTex.Initialize()) return Fail("roadMeshHighwayTex");
    if (!sTex.roadMeshStreetTex.Initialize())  return Fail("roadMeshStreetTex");
    if (!sTex.sidewalkMeshTex.Initialize())    return Fail("sidewalkMeshTex");
    if (!sTex.groundMeshTex.Initialize())      return Fail("groundMeshTex");

	if (!sInit.shadowMap.Init(4096)) return Fail("shadowMap");


	std::cout << "All renderers initialized OK.\n";


	//openGL state + error reporting
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	enableReportGlErrors();



//main application frame loop
while (!glfwWindowShouldClose(window))
{
	//GUI
	BeginGuiFrame(gui);

	// texture handling
	HandleBuildingTextureRequests(gui, sTex.urbanTex, sTex.suburbanTex, sTex.ruralTex, sTex.useUrban, sTex.useSuburban, sTex.useRural);

	//building window texture handling
	HandleWindowTextureRequests(gui, sTex.windowTex, sTex.useWindowTex);

	//building roof texture handling
	HandleRoofTextureRequests(gui, sTex.roofUrbanTex, sTex.roofSuburbanTex, sTex.roofRuralTex, sTex.useRoofUrban, sTex.useRoofSuburban, sTex.useRoofRural);


	//road, sidewalk and ground handling
	HandleSceneTextureRequests(gui, sTex.roadTex, sTex.useRoadTex, sTex.sidewalkTex, sTex.useSidewalkTex, sTex.groundTex, sTex.useGroundTex);


	//choose city radius and center based on wheter roads are shown
	CityContext city = ResolveCityContext(sRoads.showRoads, gui, sRoads.roadParams);

	glm::vec3 lightDir;
	glm::mat4 lightSpace;
	ComputeLightSpace(city, lightDir, lightSpace);


	//camera mode toggle + reset
	const bool is3D = UpdateCameraModeAndResetFromGui(gui, sInit.camera, city);
	
	// camera  and ground adjustment
	FrameContext frame = BeginFrameTimingAndVP(window, sInit.camera);

	RenderShadowPassIf3D(is3D, gui, sInit.shadowMap, shadowDepthShader, lightSpace, frame, sTex.useUrban, sTex.useSuburban, sTex.useRural,
						 sTex.useRoadTex, sTex.roadTex, sTex.useSidewalkTex, sTex.sidewalkTex, sRoads.roadMeshHighway, sRoads.roadMeshStreet,
						 sRoads.sidewalkMesh, sBuilding.buildingMesh, sTex.roadMeshStreetTex, sTex.roadMeshHighwayTex, sTex.sidewalkMeshTex,
						 sTex.buildingMeshUrban, sTex.buildingMeshSuburban, sTex.buildingMeshRural, sTex.roofMeshUrban, sTex.roofMeshSuburban,
						 sTex.roofMeshRural, sTex.windowMeshTex, sTex.groundMeshTex	);


	SetupLightingAndShadowUniforms(litShader, sInit.shadowMap, lightSpace, lightDir, is3D);


	//input + viewport
	HandlePlatformInputAndViewport(window);

	//camera per-frame update + controls
	UpdateCameraPerFrame(sInit.camera, window, frame.deltaTime, is3D);

	//draws 3D meshes (only when is3D)
	Draw3DMeshesIfEnabled(gui, is3D, lineShader, litShader, frame.viewProjection, sRoads.roadMeshStreet, sRoads.roadMeshHighway,
						 sRoads.sidewalkMesh, sBuilding.buildingMesh, sTex.buildingMeshUrban, sTex.buildingMeshSuburban, sTex.buildingMeshRural,
						 sTex.roofMeshUrban, sTex.roofMeshSuburban, sTex.roofMeshRural, sTex.windowMeshTex,
						 sTex.roadMeshStreetTex, sTex.roadMeshHighwayTex, sTex.sidewalkMeshTex, sTex.urbanTex, sTex.suburbanTex, sTex.ruralTex,
						 sTex.roofUrbanTex, sTex.roofSuburbanTex, sTex.roofRuralTex, sTex.roadTex, sTex.sidewalkTex,
						 sTex.windowTex, sTex.useUrban, sTex.useSuburban, sTex.useRural, sTex.useRoofUrban, sTex.useRoofSuburban, sTex.useRoofRural,
						 sTex.useRoadTex, sTex.useSidewalkTex, sTex.useWindowTex);

	DrawSkyboxPass(sInit.skybox, sInit.camera, frame.aspect, is3D);

	glDisable(GL_CULL_FACE);

	//ground 
	GroundContext ground = DrawGroundAndGetExtents(sInit.primitives, primShader, litShader, frame.viewProjection,
													city, sTex.useGroundTex, sTex.groundTex, sTex.groundMeshTex, sTex.groundPT);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//export request
	HandleExportIfRequested(gui, city, ground, sRoads.roadHighwayTris, sRoads.roadStreetTris, sRoads.sidewalkTris, sBuilding.buildingTriVerts, sBuilding.buildingQuadVerts, sBuilding.buildingRoofQuadVerts, sBuilding.buildingRoofTriVerts, sBuilding.buildingWindowQuadVerts, sBuilding.buildingWindowTriVerts);

	//debug lines
	DrawDebugLinesIfEnabled(sRoads.showRoads, gui, lineShader, frame.viewProjection, sRoads.highwayLines, sRoads.streetLines, sLots.lotLines, sRoads.sidewalkLines, sLots.gardenLines, sLots.footprintLines);

	// show overlay first and delay generation to next frame
	HandleGenerationOverlayAndQueue(gui);

	// draw loading overlay on top of everything
	DrawGeneratingOverlay();


	HandleQuitIfRequested(gui, window);

	EndGuiFrame(gui);

	if (gui.ConsumeGuiRecreateRequest())
	{
    gui.ShutdownGUI();
    gui.Initialize_GUI(window, "#version 330"); 
	}


	PresentAndPoll(window);

	// run generation after overlay frame is presented
	if (gGeneratePending)
	{
		gGeneratePending = false;

		GenerateCityNow(gui, sRoads.showRoads, sRoads.roadGen, sLots.lotGen, sRoads.roadParams, sRoads.roadNet, sLots.lots,
						sRoads.roadMeshHighway, sRoads.roadMeshStreet, sRoads.sidewalkMesh, sBuilding.buildingMesh,
						sTex.buildingMeshUrban, sTex.buildingMeshSuburban, sTex.buildingMeshRural,
						sTex.roadMeshHighwayTex, sTex.roadMeshStreetTex, sTex.sidewalkMeshTex,
						sRoads.highwayLines, sRoads.streetLines, sLots.lotLines, sRoads.sidewalkLines, sLots.gardenLines, sLots.footprintLines,
						sRoads.roadHighwayTris, sRoads.roadStreetTris, sRoads.sidewalkTris,
						sLots.lotLineVerts, sRoads.sidewalkLineVerts, sLots.gardenLineVerts, sLots.footprintLineVerts,
						sBuilding.buildingTriVerts, sBuilding.buildingQuadVerts,
						sTex.buildingUrbanPT, sTex.buildingSuburbanPT, sTex.buildingRuralPT,
						sTex.roadHighwayPT, sTex.roadStreetPT, sTex.sidewalkPT);

		gShowGeneratingOverlay = false;
	}



}
	//all shutdowns
	sInit.skybox.Shutdown();
	sRoads.highwayLines.Shutdown_Road();
	sRoads.streetLines.Shutdown_Road();
	sInit.primitives.Shutdown_Prim();
	sLots.lotLines.Shutdown_Road();
	sRoads.sidewalkLines.Shutdown_Road();
	sLots.gardenLines.Shutdown_Road();
	sLots.footprintLines.Shutdown_Road();
	sBuilding.buildingMesh.Shutdown();
	gui.ShutdownGUI();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}






