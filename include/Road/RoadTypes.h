#pragma once 
#include <glm/glm.hpp>
#include <cstdint>


//this is the foundation file for the road algorithm logic,
//it acts more of as a data container and separates the logic to RoadNetwork and RoadGenerator 
namespace road
{
    //fixed width intger types
    using NodeId = std::uint32_t;
    using SegId = std::uint32_t;

    //road types 
    enum class RoadType : std::uint8_t
    {
        Street = 0,
        Highway = 1
    };

    //generation phases
    enum class GenerationPhase
    {
        Streets,
        Highways
    };

    //node parameters
    struct Node
    {
        NodeId id{};
        glm::vec2 pos{};
    };

    //segment parameters
    struct Segment
    {
        SegId id{};
        NodeId a{};
        NodeId b{};
        RoadType type{RoadType::Street};
    };
}