#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace export3d
{
    struct ObjMesh
    {
        std::string name;                
        std::string material;             
        std::vector<glm::vec3> tris;      
    };

    struct ObjMaterial
    {
        std::string name;
        glm::vec3 kd;
    };

    class ObjWriter
    {
    public:
        static bool WriteMtl(const std::string& mtlPath,
                             const std::vector<ObjMaterial>& materials);

        static bool WriteObj(const std::string& objPath,
                             const std::string& mtlFileName, 
                             const std::vector<ObjMesh>& meshes);
    };
}
