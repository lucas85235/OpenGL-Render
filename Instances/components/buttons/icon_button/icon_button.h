#ifndef COMPONENTS_ICON_BUTTON_H
#define COMPONENTS_ICON_BUTTON_H

#include "native/components/base_button/base_button.h"
#include "native/components/buttons/icon_button/icon_button_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    class IconButton : public BaseButton
    {
    private:
        Future<resources::Resource> resource_future;
        const ImageAsset* asset_ptr_;

    public:
        void Setup(float size);
        void LoadIcon(const imp::resources::ResourceDefinition& resource);
        NodeHandle CreateShapeNode(AssetPtr<imp::MaterialAsset> material,
                                               AssetPtr<imp::ImageAsset> texture,
                                               MeshPtr shape_mesh);
    };

}  // namespace

#endif // COMPONENTS_ICON_BUTTON_H
