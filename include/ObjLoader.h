#ifndef GLRENDERER_OBJLOADER_H
#define GLRENDERER_OBJLOADER_H

#include "Vertex.h"

#include <string>
#include <vector>

class ObjLoader {
public:
    [[nodiscard]] static bool Load(
        const char* filePath,
        std::vector<Vertex>& vertices,
        std::vector<unsigned int>& indices
    );
};

#endif //GLRENDERER_OBJLOADER_H
