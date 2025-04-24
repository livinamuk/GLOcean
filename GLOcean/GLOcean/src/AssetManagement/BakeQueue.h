#pragma once
#include "HellTypes.h"
#include "../Types/Texture.h"

namespace BakeQueue {
    // Textures
    void QueueTextureForBaking(Texture* texture);
    void RemoveQueuedTextureBakeByJobID(int jobID);
    void ImmediateBakeAllTextures();
    const int GetQueuedTextureBakeJobCount();
    QueuedTextureBake* GetNextQueuedTextureBake();
    QueuedTextureBake* GetQueuedTextureBakeByJobID(int jobID);
}