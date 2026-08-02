// GRACE_DO_NOT_COMPILE

#ifndef COMMON_BINDLESS_H
#define COMMON_BINDLESS_H

#define STORAGE_IMAGE_BINDING 0
#define SAMPLED_IMAGE_BINDING 1
#define COMBINED_IMAGE_SAMPLER_BINDING 2
#define SAMPLER_BINDING 3
#define UNIFORM_BUFFER_BINDING 4

[[vk::binding(STORAGE_IMAGE_BINDING, 0)]]
__DynamicResource uStorageTextures[];

[[vk::binding(SAMPLED_IMAGE_BINDING, 0)]]
Texture2D uSampledTextures[];

[[vk::binding(SAMPLER_BINDING, 0)]]
SamplerState uSamplers[];

#endif