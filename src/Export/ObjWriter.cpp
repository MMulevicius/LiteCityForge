#include "Export/ObjWriter.h"
#include <fstream>
#include <cmath>

namespace export3d
{
    static glm::vec3 SafeNormalize(const glm::vec3& v)
    {
        float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        if (len < 1e-8f) return glm::vec3(0, 1, 0);
        return v / len;
    }

    bool ObjWriter::WriteMtl(const std::string& mtlPath,
                             const std::vector<ObjMaterial>& materials)
    {
        std::ofstream out(mtlPath);
        if (!out.is_open()) return false;

        out << "# LiteCityForge materials\n";
        for (const auto& m : materials)
        {
            out << "\nnewmtl " << m.name << "\n";
            out << "Ka 0 0 0\n";
            out << "Kd " << m.kd.r << " " << m.kd.g << " " << m.kd.b << "\n";
            out << "Ks 0 0 0\n";
            out << "Ns 1\n";
        }
        return true;
    }

    bool ObjWriter::WriteObj(const std::string& objPath,
                             const std::string& mtlFileName,
                             const std::vector<ObjMesh>& meshes)
    {
        std::ofstream out(objPath);
        if (!out.is_open()) return false;

        out << "# LiteCityForge OBJ export\n";
        out << "# Mixed faces: traingle for meshes quads for buildings\n";
        if (!mtlFileName.empty())
            out << "mtllib " << mtlFileName << "\n";

        int vBase = 1; // OBJ is 1-based
        int nBase = 1;

        for (const auto& mesh : meshes)
        {
            if (mesh.tris.empty() && mesh.quads.empty()) 
                continue;

            out << "\no " << mesh.name << "\n";
            out << "g " << mesh.name << "\n";
            if (!mesh.material.empty())
                out << "usemtl " << mesh.material << "\n";

            const int quadCount = (int)mesh.quads.size() / 4;
            const int triCount = (int)mesh.tris.size() / 3;

            // vertices (quads first, then tris)
            for (const auto& v : mesh.quads)
                out << "v " << v.x << " " << v.y << " " << v.z << "\n";
            for (const auto& v : mesh.tris)
                out << "v " << v.x << " " << v.y << " " << v.z << "\n";


            // normals: 1 normal per face (quads first, then tris)
            for (int q = 0; q < quadCount; ++q)
            {
                const glm::vec3& a = mesh.quads[q*4 + 0];
                const glm::vec3& b = mesh.quads[q*4 + 1];
                const glm::vec3& c = mesh.quads[q*4 + 2];
                glm::vec3 n = SafeNormalize(glm::cross(c - a, b - a));
                out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
            }

            for (int t = 0; t < triCount; ++t)
            {
                const glm::vec3& a = mesh.tris[t*3 + 0];
                const glm::vec3& b = mesh.tris[t*3 + 1];
                const glm::vec3& c = mesh.tris[t*3 + 2];
                glm::vec3 n = SafeNormalize(glm::cross(c - a, b - a));
                out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
            }

            // faces quads first (f a d c b), then tris (f v0 v2 v1)
            for (int q = 0; q < quadCount; ++q)
            {
                int v0 = vBase + q*4 + 0;
                int v1 = vBase + q*4 + 1;
                int v2 = vBase + q*4 + 2;
                int v3 = vBase + q*4 + 3;
                int vn = nBase + q;

                out << "f "
                    << v0 << "//" << vn << " "
                    << v3 << "//" << vn << " "
                    << v2 << "//" << vn << " "
                    << v1 << "//" << vn << "\n";
            }

            for (int t = 0; t < triCount; ++t)
            {
                const int vOffset = quadCount * 4;
                int v0 = vBase + vOffset + t*3 + 0;
                int v1 = vBase + vOffset + t*3 + 1;
                int v2 = vBase + vOffset + t*3 + 2;
                int vn = nBase + quadCount + t;

                out << "f "
                    << v0 << "//" << vn << " "
                    << v2 << "//" << vn << " "
                    << v1 << "//" << vn << "\n";
            }

            vBase += (int)mesh.quads.size() + (int)mesh.tris.size();
            nBase += quadCount + triCount;
        }

        return true;
    }
}
