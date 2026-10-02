#include "Rendering/CityRenderer.h"

#include <glad/glad.h>

namespace rendering
{
    namespace
    {
        // builds a textured ground quad as two triangles
        void BuildGroundPT(std::vector<road::BuildingVertexPT> &out,
                           const glm::vec2 &centerXZ,
                           float halfW,
                           float halfH,
                           float y,
                           float uvMetersPerTile)
        {
            out.clear();
            if (uvMetersPerTile <= 0.001f)
                uvMetersPerTile = 4.0f;
            const float s = 1.0f / uvMetersPerTile;

            glm::vec3 a(centerXZ.x - halfW, y, centerXZ.y - halfH);
            glm::vec3 b(centerXZ.x + halfW, y, centerXZ.y - halfH);
            glm::vec3 c(centerXZ.x + halfW, y, centerXZ.y + halfH);
            glm::vec3 d(centerXZ.x - halfW, y, centerXZ.y + halfH);

            auto UV = [&](const glm::vec3 &p)
            { return glm::vec2(p.x, p.z) * s; };

            // two tris: a-b-c, a-c-d
            out.push_back({a, UV(a)});
            out.push_back({b, UV(b)});
            out.push_back({c, UV(c)});

            out.push_back({a, UV(a)});
            out.push_back({c, UV(c)});
            out.push_back({d, UV(d)});
        }
    }

    void Draw3DMeshesIfEnabled(
        Gui &gui,
        bool is3D,
        Shader &litShader,
        const glm::mat4 &viewProjection,
        RoadContext &roads,
        BuildingContext &buildings,
        TextureContext &textures)
    {
        if (!is3D)
            return;

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        if (textures.useRoadTex && textures.roadTex != 0)
        {
            const glm::vec3 roadFallback(0.10f, 0.10f, 0.10f);
            textures.roadMeshStreetTex.Draw(litShader, viewProjection, textures.roadTex, true, roadFallback);
            textures.roadMeshHighwayTex.Draw(litShader, viewProjection, textures.roadTex, true, roadFallback);
        }
        else
        {
            litShader.use();
            glUniform1i(glGetUniformLocation(litShader.ID, "uUseTexture"), 0);

            glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.12f, 0.12f, 0.12f);
            roads.roadMeshStreet.Draw(litShader, viewProjection);

            glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.07f, 0.07f, 0.07f);
            roads.roadMeshHighway.Draw(litShader, viewProjection);
        }

        if (textures.useSidewalkTex && textures.sidewalkTex != 0)
        {
            const glm::vec3 swFallback(0.70f, 0.70f, 0.70f);
            textures.sidewalkMeshTex.Draw(litShader, viewProjection, textures.sidewalkTex, true, swFallback);
        }
        else
        {
            litShader.use();
            glUniform1i(glGetUniformLocation(litShader.ID, "uUseTexture"), 0);

            glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.70f, 0.70f, 0.70f);
            roads.sidewalkMesh.Draw(litShader, viewProjection);
        }

        const bool anyTextured = (textures.useUrban || textures.useSuburban || textures.useRural);

        if (anyTextured)
        {
            const glm::vec3 urbanFallback(0.78f, 0.78f, 0.80f);
            const glm::vec3 subFallback(0.75f, 0.75f, 0.78f);
            const glm::vec3 rurFallback(0.72f, 0.72f, 0.75f);

            textures.buildingMeshUrban.Draw(litShader, viewProjection, textures.urbanTex, textures.useUrban, urbanFallback);
            textures.buildingMeshSuburban.Draw(litShader, viewProjection, textures.suburbanTex, textures.useSuburban, subFallback);
            textures.buildingMeshRural.Draw(litShader, viewProjection, textures.ruralTex, textures.useRural, rurFallback);
        }
        else
        {
            litShader.use();
            glUniform1i(glGetUniformLocation(litShader.ID, "uUseTexture"), 0);
            glUniform3f(glGetUniformLocation(litShader.ID, "uColor"), 0.75f, 0.75f, 0.78f);
            buildings.buildingMesh.Draw(litShader, viewProjection);
        }

        // roofs
        if (gui.RenderBuildingRoofs())
        {
            glDisable(GL_CULL_FACE);

            glEnable(GL_POLYGON_OFFSET_FILL);
            // to avoid Z-fighting
            glPolygonOffset(-1.0f, -1.0f);

            const glm::vec3 roofFallback(0.65f, 0.65f, 0.67f);
            textures.roofMeshUrban.Draw(litShader, viewProjection, textures.roofUrbanTex, textures.useRoofUrban, roofFallback);
            textures.roofMeshSuburban.Draw(litShader, viewProjection, textures.roofSuburbanTex, textures.useRoofSuburban, roofFallback);
            textures.roofMeshRural.Draw(litShader, viewProjection, textures.roofRuralTex, textures.useRoofRural, roofFallback);

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

            textures.windowMeshTex.Draw(
                litShader,
                viewProjection,
                textures.windowTex,
                textures.useWindowTex,
                glm::vec3(0.20f, 0.35f, 0.55f));

            glDisable(GL_POLYGON_OFFSET_FILL);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

    // draws optional debug overlays such as roads, lots, gardens and footprints
    void DrawDebugLinesIfEnabled(
        Gui &gui,
        Shader &lineShader,
        const glm::mat4 &viewProjection,
        RoadContext &roads,
        LotContext &lots)
    {

        if (!roads.showRoads)
        {
            return;
        }

        lineShader.use();

        if (gui.ShowRoadLines())
        {
            // highways
            glLineWidth(4.0f);
            glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
            roads.highwayLines.Draw(lineShader, viewProjection);

            // streets
            glLineWidth(1.0f);
            glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
            roads.streetLines.Draw(lineShader, viewProjection);
        }
        // lots
        if (gui.ShowLotDebug())
        {
            glLineWidth(2.0f);
            glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.1f, 0.4f, 1.0f);
            lots.lotLines.Draw(lineShader, viewProjection);
            glLineWidth(1.0f);
        }

        // sidewalks
        if (gui.ShowSideWalks())
        {
            glLineWidth(2.0f);
            glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.65f, 0.65f, 0.65f);
            roads.sidewalkLines.Draw(lineShader, viewProjection);
            glLineWidth(1.0f);
        }

        // gardens
        if (gui.ShowGardens())
        {
            glLineWidth(2.0f);
            glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.1f, 0.9f, 0.2f);
            lots.gardenLines.Draw(lineShader, viewProjection);
            glLineWidth(1.0f);
        }

        // building footprints
        if (gui.ShowFootprints())
        {
            glLineWidth(2.0f);
            glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.95f, 0.65f, 0.15f);
            lots.footprintLines.Draw(lineShader, viewProjection);
            glLineWidth(1.0f);
        }
    }

    // draws he ground plane and returns its extents for export use
    GroundContext DrawGroundAndGetExtents(
        Primitives &primitives,
        Shader &primShader,
        Shader &litShader,
        const glm::mat4 &viewProjection,
        const CityContext &cityCTX,
        TextureContext &textures)
    {
        GroundContext g;
        g.margin = 20.0f;

        // ground half extents based on city size
        g.halfW = cityCTX.cityR + g.margin;
        g.halfH = cityCTX.cityR + g.margin;

        if (textures.useGroundTex && textures.groundTex != 0)
        {
            BuildGroundPT(
                textures.groundPT,
                cityCTX.centerXZ,
                g.halfW,
                g.halfH,
                0.0f, // y
                6.0f  // uv meters per tile
            );

            textures.groundMeshTex.Upload(textures.groundPT);

            textures.groundMeshTex.Draw(litShader, viewProjection, textures.groundTex, textures.groundTex != 0, glm::vec3(0.0f, 0.30f, 0.0f));
        }
        else
        {
            primitives.DrawGround(primShader, viewProjection, cityCTX.centerXZ, g.halfW, g.halfH);
        }

        return g;
    }

    // renders the depth-only shadow pass from the light's perspective
    void RenderShadowPassIf3D(
        bool is3D,
        Gui &gui,
        ShadowMap &shadowMap,
        Shader &shadowDepthShader,
        const glm::mat4 &lightSpace,
        const FrameContext &frame,
        RoadContext &roads,
        BuildingContext &buildings,
        TextureContext &textures)
    {
        if (!is3D)
            return;

        shadowMap.BeginDepthPass();

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.5f, 4.0f);

        shadowDepthShader.use();

        const bool anyBuildingTex = (textures.useUrban || textures.useSuburban || textures.useRural);

        // roads
        if (textures.useRoadTex && textures.roadTex != 0)
        {
            textures.roadMeshStreetTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
            textures.roadMeshHighwayTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
        }
        else
        {
            roads.roadMeshHighway.Draw(shadowDepthShader, lightSpace);
            roads.roadMeshStreet.Draw(shadowDepthShader, lightSpace);
        }

        // sidewalks
        if (textures.useSidewalkTex && textures.sidewalkTex != 0)
        {
            textures.sidewalkMeshTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
        }
        else
        {
            roads.sidewalkMesh.Draw(shadowDepthShader, lightSpace);
        }

        // building bases
        if (anyBuildingTex)
        {
            textures.buildingMeshUrban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
            textures.buildingMeshSuburban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
            textures.buildingMeshRural.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
        }
        else
        {
            buildings.buildingMesh.Draw(shadowDepthShader, lightSpace);
        }

        // roofs
        if (gui.RenderBuildingRoofs())
        {
            textures.roofMeshUrban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
            textures.roofMeshSuburban.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
            textures.roofMeshRural.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
        }

        // windows
        if (gui.RenderBuildingWindows())
        {
            textures.windowMeshTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));
        }

        // ground
        textures.groundMeshTex.Draw(shadowDepthShader, lightSpace, 0, false, glm::vec3(0));

        glDisable(GL_POLYGON_OFFSET_FILL);

        shadowMap.EndDepthPass(frame.w, frame.h);
    }

}