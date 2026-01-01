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
        out << "# Triangles only (3 verts per face)\n";
        if (!mtlFileName.empty())
            out << "mtllib " << mtlFileName << "\n";

        int vBase = 1; // OBJ is 1-based
        int nBase = 1;

        for (const auto& mesh : meshes)
        {
            if (mesh.tris.empty()) continue;

            out << "\no " << mesh.name << "\n";
            out << "s off\n";
            if (!mesh.material.empty())
                out << "usemtl " << mesh.material << "\n";

            // vertices
            for (const auto& v : mesh.tris)
                out << "v " << v.x << " " << v.y << " " << v.z << "\n";

            // normals: 1 normal per triangle
            const int triCount = (int)mesh.tris.size() / 3;
            for (int t = 0; t < triCount; ++t)
            {
                const glm::vec3& a = mesh.tris[t*3 + 0];
                const glm::vec3& b = mesh.tris[t*3 + 1];
                const glm::vec3& c = mesh.tris[t*3 + 2];
                glm::vec3 n = SafeNormalize(glm::cross(b - a, c - a));
                out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
            }

            // faces
            for (int t = 0; t < triCount; ++t)
            {
                int v0 = vBase + t*3 + 0;
                int v1 = vBase + t*3 + 1;
                int v2 = vBase + t*3 + 2;
                int vn = nBase + t;

                // f v//vn v//vn v//vn
                out << "f "
                    << v0 << "//" << vn << " "
                    << v1 << "//" << vn << " "
                    << v2 << "//" << vn << "\n";
            }

            vBase += (int)mesh.tris.size();
            nBase += triCount;
        }

        return true;
    }
}
