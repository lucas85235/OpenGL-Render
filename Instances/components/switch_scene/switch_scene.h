#ifndef COMPONENTS_SWITCH_SCENE_H
#define COMPONENTS_SWITCH_SCENE_H

#include "imp.h"
#include "native/components/fade/fade.h"
#include "native/components/skybox/skybox.h"
#include "native/components/switch_scene/switch_scene.h"

namespace ix::samsung::homecomponents
{
using namespace imp;

struct SwitchSceneFinished:Event{

};

struct OnSceneFinishedLoading:Event{
    NodeHandle loadedNode;
};

struct OnScenesLoadFinished : Event {
};

struct SceneData {
    public:
        resources::ResourceDefinition scene;
        resources::ResourceDefinition skybox;
        bool active;
};

class SwitchScene : public Component
{
    public:
        void Setup(const std::vector<SceneData>& scenes);
        void LoadScene(std::vector<resources::ResourceDefinition> scene);
        void SwitchToScene(int index);
        void DisableAllScenes();
        void OnSwitchSceneFinished();
        std::unordered_map<int, NodeHandle> GetLoadedSceneNodes();

    private:
        std::map<int, NodeHandle> scenes_;
        ComponentHandle<Fade> fade_component_;
        ComponentHandle<Skybox> skybox_component_;
        std::unordered_map<int, NodeHandle> scene_nodes_;
        int loaded_scenes_counter_ = 0;
        int index_;
        std::vector<resources::ResourceDefinition> background_images_;
};

}  // namespace ix::samsung::homecomponents

#endif  // COMPONENTS_SWITCH_SCENE_H
