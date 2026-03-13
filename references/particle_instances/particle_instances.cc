#include <memory>
#include <optional>
#include <utility>
#include <filesystem>
#include <tuple>
#include <random>
#include <iostream>

#include "imp.h"
#include "core/ncsb/view_attachment_manager.h"
#include "native/components/particle_instances/particle_instances_assets.h"
#include "native/components/particle_instances/particle_instances.h"

#if IMP_RUNTIME(DEV)
#include "core/common/debug_draw.h"
#endif

namespace ix::samsung::homecomponents {
    void ParticleInstances::Setup() {
        // Empty setup is necessary to prevent crash because it is using proto
    }

    void ParticleInstances::Setup(ParticleInstancesParams &params) {
        camera_node_ = GetView().GetCameraManager().GetCamera()->GetNode();
        info_ = params.info;
        InitializeProto();

        filament::Engine &engine = *GetView().GetSharedEngine();
        filament::Scene &scene = *GetView().GetGroupsManager().GetScene("Main");

        positionFunction = params.positionFunction;
        particles_amount = params.amount;
        texture_width = std::ceil(std::sqrt(particles_amount));

        Vertex *vertices = params.mesh->GetVertices();
        uint16_t *indexes = params.mesh->GetIndexes();

        app.vb = filament::VertexBuffer::Builder()
                .vertexCount(params.mesh->GetVerticesAmount())
                .bufferCount(1)
                .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT3, 0, 5 * sizeof(float))
                .attribute(filament::VertexAttribute::UV0, 0, filament::VertexBuffer::AttributeType::FLOAT2, 3 * sizeof(float), 5 * sizeof(float))
                .build(engine);
        app.ib = filament::IndexBuffer::Builder()
                .indexCount(params.mesh->GetIndexesAmount())
                .bufferType(filament::IndexBuffer::IndexType::USHORT)
                .build(engine);

        app.vb->setBufferAt(engine, 0, filament::VertexBuffer::BufferDescriptor(vertices, sizeof(Vertex) * params.mesh->GetVerticesAmount(), nullptr));
        app.ib->setBuffer(engine, filament::IndexBuffer::BufferDescriptor(indexes, sizeof(uint16_t) * params.mesh->GetIndexesAmount(), nullptr));


        auto mapping_texture_ = CreateMappingTexture();
        material = std::move(params.material);
        auto text = GetView().GetTextureFactory().WrapTexture(mapping_texture_);

        material->SetParameter("Tex", std::move(text));
        material->SetParameter("TextureWidth", texture_width);
        material->SetParameter("Amount", params.amount);

        if (params.texture != nullptr)
            material->TrySetParameter("Texture", std::move(params.texture));

        material->TrySetParameter("Velocity", params.velocity);
        material->TrySetParameter("Angle", params.angle);
        material->TrySetParameter("Size", params.size);
        material->TrySetParameter("Radius", params.radius);
        material->TrySetParameter("Position", params.position);
        material->TrySetParameter("Rotation", params.rotation);
        material->TrySetParameter("Color", params.color);
        material->TrySetParameter("Center", params.center);

        app.matInstance = material->GetFilamentMaterialInstance();
        entity_ = utils::EntityManager::get().create();

        filament::RenderableManager::Builder(1)
                .boundingBox({{ 0, 0, 0 }, { 0, 0, 0 }})
                .material(0, app.matInstance)
                .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, app.vb, app.ib)
                .culling(false)
                .receiveShadows(false)
                .castShadows(false)
                .instances(particles_amount)
                .build(engine, entity_);

        auto &vam = ViewAttachmentManager::GetInstance();
        vam.Add(entity_, &GetView());

        app.tm = &engine.getTransformManager();
        auto &rm = engine.getRenderableManager();

        if (!app.tm->hasComponent(entity_)) {
            app.tm->create(entity_);
        }

        SetTransform(info_);
        scene.addEntity(entity_);

        auto instance = GetInstance();
        layer_mask_ = GetView().GetRenderableManager().GetLayerMask(instance);

        GetView().GetRenderableManager().SetLayerMask(GetInstance(), 0xff, 0);
    }

    void ParticleInstances::OnActiveStatusChanged(bool is_active) {
        GetView().GetRenderableManager().SetLayerMask(GetInstance(), 0xff,
                                                      is_active ? layer_mask_ : 0);
    }

    filament::RenderableManager::Instance ParticleInstances::GetInstance() const {
        return GetView().GetRenderableManager().GetInstance(entity_);
    }

    void ParticleInstances::Cleanup() {
        filament::Engine &engine = *GetView().GetSharedEngine();
        utils::EntityManager::get().destroy(entity_);
        engine.destroy(app.renderable);
        engine.destroy(app.matInstance);
        engine.destroy(app.vb);
        engine.destroy(app.ib);
        material.release();
    }

    filament::Texture *ParticleInstances::CreateMappingTexture() {
        int width = texture_width;
        int height = texture_width;
        int quant = pow(texture_width, 2);

        float *value_data = new float[quant * 4];
        //std::fill(value_data, value_data + texture_width * texture_width * particles_amount, 0);

        for (int i = 0; i < quant; ++i) {
            if (i < particles_amount) {
                float t = float(i) / particles_amount;
                auto x1 = positionFunction(t);
                value_data[i * 4 + 0] = x1.r;
                value_data[i * 4 + 1] = x1.g;
                value_data[i * 4 + 2] = x1.b;
                value_data[i * 4 + 3] = x1.a;
            } else {
                value_data[i * 4 + 0] = 0;
                value_data[i * 4 + 1] = 0;
                value_data[i * 4 + 2] = 0;
                value_data[i * 4 + 3] = 0;
            }
        }

        filament::Texture::PixelBufferDescriptor buffer(value_data, sizeof(float) * 4 * quant, filament::Texture::Format::RGBA,
                                                        filament::Texture::Type::FLOAT, [](void *buffer, size_t size, void *user) {
                    delete[] static_cast<uint32_t *>(buffer);
                });

        filament::Texture *texture = filament::Texture::Builder()
                .width(texture_width)
                .height(texture_width)
                .levels(1)
                .format(filament::Texture::InternalFormat::RGBA32F)
                .build(*GetView().GetSharedEngine());

        texture->setImage(*GetView().GetSharedEngine(), 0, std::move(buffer));

        return texture;
    }

    void ParticleInstances::InitializeProto() {
        state_.position.x = state_.position.x.value_or(info_.position.x);
        state_.position.y = state_.position.y.value_or(info_.position.y);
        state_.position.z = state_.position.z.value_or(info_.position.z);

        state_.rotation.x = state_.rotation.x.value_or(info_.rotation.x);
        state_.rotation.y = state_.rotation.y.value_or(info_.rotation.y);
        state_.rotation.z = state_.rotation.z.value_or(info_.rotation.z);

        state_.scale.x = state_.scale.x.value_or(info_.scale.x);
        state_.scale.y = state_.scale.y.value_or(info_.scale.y);
        state_.scale.z = state_.scale.z.value_or(info_.scale.z);
    }

    void ParticleInstances::OnIsfStateChanged() {
        info_.position.x = state_.position.x.value();
        info_.position.y = state_.position.y.value();
        info_.position.z = state_.position.z.value();

        info_.rotation.x = state_.rotation.x.value();
        info_.rotation.y = state_.rotation.y.value();
        info_.rotation.z = state_.rotation.z.value();

        info_.scale.x = state_.scale.x.value();
        info_.scale.y = state_.scale.y.value();
        info_.scale.z = state_.scale.z.value();

        if (app.tm == nullptr)
            return;

        SetTransform(info_);
    }

    void ParticleInstances::SetTransform(EntityInfo info)
    {
        output::Info("%f, %f, %f", info.rotation.x, info.rotation.y, info.rotation.z);

        // Converter os ângulos de graus para radianos
        float rotationX = info.rotation.x * (M_PI / 180.0f);
        float rotationY = info.rotation.y * (M_PI / 180.0f);
        float rotationZ = info.rotation.z * (M_PI / 180.0f);

        // Criar matrizes de rotação para os eixos X, Y e Z
        mat4f rotationMatrixX = mat4f::rotation(rotationX, float4{1.0f, 0.0f, 0.0f, 0.0f});
        mat4f rotationMatrixY = mat4f::rotation(rotationY, float4{0.0f, 1.0f, 0.0f, 0.0f});
        mat4f rotationMatrixZ = mat4f::rotation(rotationZ, float4{0.0f, 0.0f, 1.0f, 0.0f});

        // Combinar as rotações (Z * Y * X)
        mat4f combinedRotation = rotationMatrixZ * rotationMatrixY * rotationMatrixX;

        auto transform =
                filament::math::mat4f::translation(info.position) *
                filament::math::mat4f::scaling(info.scale) *
                combinedRotation;

        app.tm->setTransform(app.tm->getInstance(entity_), transform);
    }

    void ParticleInstances::Update(const FrameTime &frame_time) {
        material->TrySetParameter("CameraPosition", camera_node_->GetWorldPosition() - info_.position);
    }

}  // namespace ix::samsung::homecomponents
