#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>


namespace road
{
    using BlockId  = std::uint32_t;

    struct Block
    {
        BlockId id{};
        std::vector<glm::vec2> boundary;
        float area = 0.0f;
    };

}