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

	//building mesh variable
	road::BuildingRenderer buildingMesh;
	road::BuildingRenderer roofMesh;
	road::BuildingRenderer windowMesh;


	//building window texture variables
	road::BuildingTexturedRenderer windowMeshTex;

	//roof texture variables
	road::BuildingTexturedRenderer roofMeshUrban;
	road::BuildingTexturedRenderer roofMeshSuburban;
	road::BuildingTexturedRenderer roofMeshRural;
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




	//building detail tris
	std::vector<glm::vec3> buildingTriVerts;
	std::vector<glm::vec3> buildingRoofTriVerts;
	std::vector<glm::vec3> buildingWindowTriVerts;

	//building detail quads
	std::vector<glm::vec3> buildingQuadVerts;
	std::vector<glm::vec3> buildingRoofQuadVerts;
	std::vector<glm::vec3> buildingWindowQuadVerts;

	//window texture
	GLuint windowTex = 0;
	GLuint roadTex = 0;
	GLuint sidewalkTex = 0;
	GLuint groundTex = 0;

	bool useWindowTex = false;
	bool useRoadTex = false;
	bool useSidewalkTex = false;
	bool useGroundTex = false;
	

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


		// Roof details
		road::BuildRoofDetailVerts(lots, buildingRoofQuadVerts, buildingRoofTriVerts, baseY, floorH);

		road::BuildRoofDetailTriVertsTexturedByZone(
			lots,
			roofUrbanPT, roofSuburbanPT, roofRuralPT,
			baseY, floorH,
			2.0f
		);

		roofMeshUrban.Upload(roofUrbanPT);
		roofMeshSuburban.Upload(roofSuburbanPT);
		roofMeshRural.Upload(roofRuralPT);



		// Windows (quads)
		road::BuildWindowDetailQuads(lots, buildingWindowQuadVerts, baseY, floorH);

		// Convert window quads -> tris for rendering with BuildingRenderer
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

		// append any roof tris (rural gable end caps etc.)
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
		windowMeshTex.Upload(windowPTTris);



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

	void SafeDeleteTexture(GLuint& tex)
	{
		if (tex != 0)
		{
			glDeleteTextures(1, &tex);
			tex = 0;
		}
	}

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


	void HandleBuildingTextureRequests(
		Gui& gui,
		GLuint& urbanTex, GLuint& suburbanTex, GLuint& ruralTex,
		bool& useUrban, bool& useSuburban, bool& useRural)
	{
		std::string path;

		// Urban
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

		// Suburban
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

		// Rural
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

	void Draw3DMeshesIfEnabled (Gui& gui, bool is3D, Shader& lineShader, Shader& buildingTexShader,
										const glm::mat4& viewProjection,
										road::BuildingRenderer& roadMeshStreet,
										road::BuildingRenderer& roadMeshHighway,
										road::BuildingRenderer& sidewalkMesh,
										road::BuildingRenderer& buildingMesh,
										road::BuildingTexturedRenderer& buildingMeshUrban,
										road::BuildingTexturedRenderer& buildingMeshSuburban,
										road::BuildingTexturedRenderer& buildingMeshRural,
										road::BuildingTexturedRenderer& roadMeshStreetTex,
										road::BuildingTexturedRenderer& roadMeshHighwayTex,
										road::BuildingTexturedRenderer& sidewalkMeshTex,

										GLuint urbanTex, GLuint suburbanTex, GLuint ruralTex,
										GLuint roofUrbanTex, GLuint roofSuburbanTex, GLuint roofRuralTex,
										GLuint roadTex, GLuint sidewalkTex,
										bool useUrban, bool useSuburban, bool useRural,
										bool useRoofUrban, bool useRoofSuburban, bool useRoofRural,
										bool useRoadTex, bool useSidewalkTex) 
	{
			if (!is3D)
			{
				return;
			}

			glDisable(GL_CULL_FACE); 
			lineShader.use();

            // Roads / Sidewalks
            if (useRoadTex)
            {
                const glm::vec3 roadFallback(0.10f, 0.10f, 0.10f);
                roadMeshStreetTex.Draw(buildingTexShader, viewProjection, roadTex, true, roadFallback);
                roadMeshHighwayTex.Draw(buildingTexShader, viewProjection, roadTex, true, roadFallback);
            }
            else
            {
                lineShader.use();
                glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.12f, 0.12f, 0.12f);
                roadMeshStreet.Draw(lineShader, viewProjection);

                glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.07f, 0.07f, 0.07f);
                roadMeshHighway.Draw(lineShader, viewProjection);
            }

            if (useSidewalkTex)
            {
                const glm::vec3 swFallback(0.70f, 0.70f, 0.70f);
                sidewalkMeshTex.Draw(buildingTexShader, viewProjection, sidewalkTex, true, swFallback);
            }
            else
            {
                lineShader.use();
                glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.70f, 0.70f, 0.70f);
                sidewalkMesh.Draw(lineShader, viewProjection);
            }

			//buildings
			const bool anyTextured = (useUrban || useSuburban || useRural);

			// 1) Draw the building base FIRST
			if (anyTextured)
			{
				const glm::vec3 urbanFallback(0.78f, 0.78f, 0.80f);
				const glm::vec3 subFallback  (0.75f, 0.75f, 0.78f);
				const glm::vec3 rurFallback  (0.72f, 0.72f, 0.75f);

				buildingMeshUrban.Draw(buildingTexShader, viewProjection, urbanTex, useUrban, urbanFallback);
				buildingMeshSuburban.Draw(buildingTexShader, viewProjection, suburbanTex, useSuburban, subFallback);
				buildingMeshRural.Draw(buildingTexShader, viewProjection, ruralTex, useRural, rurFallback);
			}
			else
			{
				lineShader.use();
				glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.75f, 0.75f, 0.78f);
				buildingMesh.Draw(lineShader, viewProjection);
			}

			// Roofs AFTER buildings
			if (gui.RenderBuildingRoofs())
			{
				const glm::vec3 roofFallback(0.65f, 0.65f, 0.67f);

				roofMeshUrban.Draw(buildingTexShader, viewProjection,
								roofUrbanTex, useRoofUrban, roofFallback);

				roofMeshSuburban.Draw(buildingTexShader, viewProjection,
									roofSuburbanTex, useRoofSuburban, roofFallback);

				roofMeshRural.Draw(buildingTexShader, viewProjection,
								roofRuralTex, useRoofRural, roofFallback);
			}


			// 3) Draw windows LAST (so they sit on top)
			if (gui.RenderBuildingWindows())
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				// keep depth test ON, but don't write depth (good for overlays)
				glDepthMask(GL_FALSE);

				windowMeshTex.Draw(buildingTexShader, viewProjection,
								windowTex,
								useWindowTex,
								glm::vec3(0.20f, 0.35f, 0.55f));

				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}




			if(anyTextured)
			{

				const glm::vec3 urbanFallback(0.78f, 0.78f, 0.80f);
				const glm::vec3 subFallback  (0.75f, 0.75f, 0.78f);
				const glm::vec3 rurFallback  (0.72f, 0.72f, 0.75f);

				buildingMeshUrban.Draw(buildingTexShader, viewProjection, urbanTex, useUrban, urbanFallback);
				buildingMeshSuburban.Draw(buildingTexShader, viewProjection, suburbanTex, useSuburban, subFallback);
				buildingMeshRural.Draw(buildingTexShader, viewProjection, ruralTex, useRural, rurFallback);
			}
			else
			{
				lineShader.use();
				glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.75f, 0.75f, 0.78f);
				buildingMesh.Draw(lineShader, viewProjection);
			}
		
	}
	//ground and returns for export
	GroundContext DrawGroundAndGetExtents(
		Primitives& primitives,
		Shader& primShader,
		Shader& buildingTexShader,
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
			// Build a simple textured quad (2 tris) each frame (6 verts only, cheap)
			BuildGroundPT(
				groundPT,
				cityCTX.centerXZ,
				g.halfW,
				g.halfH,
				0.0f,      // y
				6.0f       // uv meters per tile (tweak later)
			);

			groundMeshTex.Upload(groundPT);

			// fallback color used if texture missing, but we already check it above
			groundMeshTex.Draw(buildingTexShader, viewProjection, groundTex, true, glm::vec3(0.15f, 0.25f, 0.15f));
		}
		else
		{
			primitives.DrawGround(primShader, viewProjection, cityCTX.centerXZ, g.halfW, g.halfH);
		}

		return g;
	}


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
			ex.buildingQuads   = buildingQuadVerts;

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

			// build PT buffers (UVs) + upload textured renderers
			ConvertTriVertsToPT_WorldXZ(roadHighwayTris, roadHighwayPT, 3.0f);
			ConvertTriVertsToPT_WorldXZ(roadStreetTris,  roadStreetPT,  3.0f);
			ConvertTriVertsToPT_WorldXZ(sidewalkTris,    sidewalkPT,    2.0f);

			roadMeshHighwayTex.Upload(roadHighwayPT);
			roadMeshStreetTex.Upload(roadStreetPT);
			sidewalkMeshTex.Upload(sidewalkPT);


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
				lotLineVerts, sidewalkLineVerts, gardenLineVerts, footprintLineVerts, 
				buildingTriVerts, buildingQuadVerts, buildingUrbanPT, buildingSubUrbanPT,
				buildingRuralPT, lotLines, sidewalkLines, gardenLines, footprintLines,
				buildingMesh, roofMesh, windowMesh,
				buildingMeshUrban, buildingMeshSuburban, buildingMeshRural,
				buildingRoofQuadVerts, buildingRoofTriVerts, buildingWindowQuadVerts,
				buildingWindowTriVerts);

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


	//roads and sidewalks mesh variables
	road::BuildingRenderer roadMeshHighway;
	road::BuildingRenderer roadMeshStreet;
	road::BuildingRenderer sidewalkMesh;

	std::vector<glm::vec3> roadHighwayTris;
	std::vector<glm::vec3> roadStreetTris;
	std::vector<glm::vec3> sidewalkTris;

	//building texture variables
	road::BuildingTexturedRenderer buildingMeshUrban;
	road::BuildingTexturedRenderer buildingMeshSuburban;
	road::BuildingTexturedRenderer buildingMeshRural;

	std::vector<road::BuildingVertexPT> buildingUrbanPT;
	std::vector<road::BuildingVertexPT> buildingSuburbanPT;
	std::vector<road::BuildingVertexPT> buildingRuralPT;

	GLuint urbanTex = 0, suburbanTex = 0, ruralTex = 0;
	bool useUrban = false, useSuburban = false, useRural = false;

	GLuint roofUrbanTex = 0, roofSuburbanTex = 0, roofRuralTex = 0;
	bool useRoofUrban = false, useRoofSuburban = false, useRoofRural = false;



	
	//three shaders: one for primitives (such as ground), line rendering of triangles, building textures
	Shader primShader("../include/basic.vert", "../include/basic.frag");
	Shader lineShader("../include/line.vert", "../include/line.frag");
	Shader buildingTexShader("../include/building_tex.vert", "../include/building_tex.frag");

	//Skybox initilization
	Skybox skybox;
	if (!skybox.Initialize("../assets/textures/SkyBox/Standard-Cube-Map"))
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
	if (!highwayLines.Initialize_Road())   return Fail("highwayLines");
	if (!streetLines.Initialize_Road())    return Fail("streetLines");
	if (!lotLines.Initialize_Road())       return Fail("lotLines");
	if (!sidewalkLines.Initialize_Road())  return Fail("sidewalkLines");
	if (!gardenLines.Initialize_Road())    return Fail("gardenLines");
	if (!footprintLines.Initialize_Road()) return Fail("footprintLines");

	// non-textured meshes
	if (!buildingMesh.Initialize())        return Fail("buildingMesh");
	if (!roadMeshHighway.Initialize())     return Fail("roadMeshHighway");
	if (!roadMeshStreet.Initialize())      return Fail("roadMeshStreet");
	if (!sidewalkMesh.Initialize())        return Fail("sidewalkMesh");

	// textured building meshes (urban/suburban/rural)
	if (!buildingMeshUrban.Initialize())   return Fail("buildingMeshUrban");
	if (!buildingMeshSuburban.Initialize())return Fail("buildingMeshSuburban");
	if (!buildingMeshRural.Initialize())   return Fail("buildingMeshRural");

	// details
	if (!roofMesh.Initialize())            return Fail("roofMesh");
	if (!windowMesh.Initialize())          return Fail("windowMesh");

	// textured windows (alpha windows)
	if (!windowMeshTex.Initialize())       return Fail("windowMeshTex");

	// textured roofs
	if (!roofMeshUrban.Initialize())    return Fail("roofMeshUrban");
	if (!roofMeshSuburban.Initialize()) return Fail("roofMeshSuburban");
	if (!roofMeshRural.Initialize())    return Fail("roofMeshRural");

	// road, sidewalk and ground texture
	if (!roadMeshHighwayTex.Initialize()) return Fail("roadMeshHighwayTex");
    if (!roadMeshStreetTex.Initialize())  return Fail("roadMeshStreetTex");
    if (!sidewalkMeshTex.Initialize())    return Fail("sidewalkMeshTex");
    if (!groundMeshTex.Initialize())      return Fail("groundMeshTex");



	std::cout << "All renderers initialized OK.\n";


	//openGL state + error reporting
	glEnable(GL_DEPTH_TEST);
	enableReportGlErrors();


//the frame loop
while (!glfwWindowShouldClose(window))
{
	//GUI
	BeginGuiFrame(gui);

	// texture handling
	HandleBuildingTextureRequests(gui, urbanTex, suburbanTex, ruralTex, useUrban, useSuburban, useRural);

	//building window texture handling
	HandleWindowTextureRequests(gui, windowTex, useWindowTex);

	//building roof texture handling
	HandleRoofTextureRequests(gui,roofUrbanTex, roofSuburbanTex, roofRuralTex,useRoofUrban, useRoofSuburban, useRoofRural);


	//road, sidewalk and ground handling
	HandleSceneTextureRequests(gui, roadTex, useRoadTex, sidewalkTex, useSidewalkTex, groundTex, useGroundTex);


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
	Draw3DMeshesIfEnabled(gui, is3D, lineShader, buildingTexShader, frame.viewProjection, roadMeshStreet,
						  roadMeshHighway, sidewalkMesh, buildingMesh, buildingMeshUrban,
						  buildingMeshSuburban, buildingMeshRural,roadMeshStreetTex,roadMeshHighwayTex,
						  sidewalkMeshTex, urbanTex, suburbanTex,
						  ruralTex, roofUrbanTex, roofSuburbanTex, roofRuralTex, roadTex, sidewalkTex,
						  useUrban, useSuburban, useRural,
						  useRoofUrban, useRoofSuburban, useRoofRural, useRoadTex, useSidewalkTex);

	//ground 
	GroundContext ground = DrawGroundAndGetExtents(
		primitives,
		primShader,
		buildingTexShader,
		frame.viewProjection,
		city,
		useGroundTex,
		groundTex,
		groundMeshTex,
		groundPT
	);

	//export request
	HandleExportIfRequested(gui, city, ground, roadHighwayTris, roadStreetTris, sidewalkTris, buildingTriVerts, buildingQuadVerts, buildingRoofQuadVerts, buildingRoofTriVerts, buildingWindowQuadVerts, buildingWindowTriVerts);

	//debug lines
	DrawDebugLinesIfEnabled(showRoads, gui, lineShader, frame.viewProjection, highwayLines, streetLines, lotLines, sidewalkLines, gardenLines, footprintLines);

	GenerateCityIfRequested(gui, showRoads, roadGen, lotGen, roadParams, roadNet, lots,
    						roadMeshHighway, roadMeshStreet, sidewalkMesh, buildingMesh,
							buildingMeshUrban, buildingMeshSuburban, buildingMeshRural,
							roadMeshHighwayTex, roadMeshStreetTex, sidewalkMeshTex,
    						highwayLines, streetLines, lotLines, sidewalkLines, gardenLines, footprintLines,
    						roadHighwayTris, roadStreetTris, sidewalkTris,
    						lotLineVerts, sidewalkLineVerts, gardenLineVerts, footprintLineVerts, buildingTriVerts, buildingQuadVerts,
							buildingUrbanPT, buildingSuburbanPT, buildingRuralPT, roadHighwayPT, roadStreetPT, sidewalkPT);

	HandleQuitIfRequested(gui, window);

	EndGuiFrame(gui);

	if (gui.ConsumeGuiRecreateRequest())
	{
    gui.ShutdownGUI();
    gui.Initialize_GUI(window, "#version 330"); 
	}


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






