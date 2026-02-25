#ifndef COMPONENTS_TEXTPRO_H
#define COMPONENTS_TEXTPRO_H

#include <iostream>
#include <string>
#include <functional>
#include "core/ncsb/component.h"
#include "absl/status/status.h"
#include "native/components/text_pro/text_pro_assets.h"
#include "proto/components/text_pro_state.proto.imp.h"
#include "imp.h"

#if IMP_RUNTIME(DEV)
#include "dear_imgui/imgui.h"
#include "core/common/log.h"
#endif

using namespace std;
using namespace imp;

namespace ix::samsung::homecomponents {

    class TextPro : public Component {
    private:
        void CreatePanel(const resources::ResourceDefinition &image, const resources::ResourceDefinition &mat, float size = 1.0f);
        TexturePtr CreateMappingTexture();
        NodeHandle CreateShapeNode(const AssetPtr <imp::MaterialAsset> &material,
                                   const AssetPtr <imp::ImageAsset> &texture,
                                   MeshPtr shape_mesh);
        imp::TexturePtr mapping_texture_;
        int char_length_ = 0;
        vector<char> value_data;
        std::string chars_;
        float font_size_ = 8.0;
        TextProState state_;
        MaterialPtr text_material_;
        NodeHandle quad_node_;

        //default values
        float default_font_size_ = 2.0;
        float max_soft_edge_ = 0.5;
        float min_soft_edge_ = 0.45;

    public:

        void Setup();
        void SetText(const string &text, float size = 1.0);
        void SetTextPostInitialized(string text);
        void SetFontSize(float size);
        using IsfInfo = imp::IsfInfo<&TextPro::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };

}  // namespace ix::moohan::home_support
#endif // COMPONENTS_TEXTPRO_H