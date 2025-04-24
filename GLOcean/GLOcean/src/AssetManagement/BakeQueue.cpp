#include "BakeQueue.h"
#include "AssetManager.h"
#include "../API/OpenGL/GL_backend.h"

namespace BakeQueue {

    std::vector<QueuedTextureBake> g_queuedTextureBakes;
    int g_textureBakeJobID = 0;

    void QueueTextureForBaking(Texture* texture) {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        for (int i = 0; i < texture->GetTextureDataCount(); i++) {
            QueuedTextureBake& queuedTextureBake = g_queuedTextureBakes.emplace_back();
            queuedTextureBake.jobID = g_textureBakeJobID++;
            queuedTextureBake.texture = texture;
            queuedTextureBake.width = texture->GetWidth(i);
            queuedTextureBake.height = texture->GetHeight(i);
            queuedTextureBake.format = texture->GetFormat();
            queuedTextureBake.internalFormat = texture->GetInternalFormat();
            queuedTextureBake.data = texture->GetData(i);
            queuedTextureBake.dataSize = texture->GetDataSize(i);
            queuedTextureBake.mipmapLevel = i;
        }
    }

    void RemoveQueuedTextureBakeByJobID(int jobID) {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        for (int i = 0; i < g_queuedTextureBakes.size(); i++) {
            if (g_queuedTextureBakes[i].jobID == jobID) {
                g_queuedTextureBakes.erase(g_queuedTextureBakes.begin() + i);
                return;
            }
        }
    }

    void ImmediateBakeAllTextures() {
        while (g_queuedTextureBakes.size()) {
            OpenGLBackend::ImmediateBake(g_queuedTextureBakes[0]);
        }
    }

    QueuedTextureBake* GetNextQueuedTextureBake() {
        for (QueuedTextureBake& queuedTextureBake : g_queuedTextureBakes) {
            if (!queuedTextureBake.inProgress) {
                //OpenGLBackend::AsyncBakeQueuedTextureBake(queuedTextureBake);
                return &queuedTextureBake;
            }
        }
        return nullptr;
    }

    const int GetQueuedTextureBakeJobCount() {
        return g_queuedTextureBakes.size();
    }

    QueuedTextureBake* GetQueuedTextureBakeByJobID(int jobID) {
        for (QueuedTextureBake& queuedTextureBake : g_queuedTextureBakes) {
            if (queuedTextureBake.jobID == jobID) {
                return &queuedTextureBake;
            }
        }
        std::cout << "BakeQueue::GetQueuedTextureBakeByJobID(int jobID) failed. '" << jobID << "' did not exist\n";
        return nullptr;
    }
}