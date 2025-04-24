#pragma once

enum class LoadingState {
    AWAITING_LOADING_FROM_DISK,
    LOADING_FROM_DISK,
    LOADING_COMPLETE
};

enum class BakeState {
    AWAITING_BAKE,
    BAKING_IN_PROGRESS,
    BAKE_COMPLETE,
    UNDEFINED
};

enum class BlendingMode { 
    NONE, 
    BLENDED,
    ALPHA_DISCARDED,
    HAIR_UNDER_LAYER,
    HAIR_TOP_LAYER,
    DO_NOT_RENDER 
};

enum class ImageDataType {
    UNCOMPRESSED,
    COMPRESSED,
    EXR,
    UNDEFINED
};

enum class TextureWrapMode {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    UNDEFINED
};

enum class TextureFilter {
    NEAREST,
    LINEAR,
    LINEAR_MIPMAP,
    UNDEFINED
};