#ifndef GLRENDERER_GLTFLOADER_H
#define GLRENDERER_GLTFLOADER_H

#include "RenderModel.h"

class GltfLoader {
public:
    [[nodiscard]] static bool Load(
        const char* filePath,
        RenderModel& model
    );
};

#endif //GLRENDERER_GLTFLOADER_H
