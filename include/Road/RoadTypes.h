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

    enum class RoadType : std::uint8_t
    {
        Street = 0,
        Highway = 1
    };

    enum class GenerationPhase
    {
        Streets,
        Highways
    };

    struct Node
    {
        NodeId id{};
        glm::vec2 pos{};
    };

    struct Segment
    {
        SegId id{};
        NodeId a{};
        NodeId b{};
        RoadType type{RoadType::Street};
    };
}