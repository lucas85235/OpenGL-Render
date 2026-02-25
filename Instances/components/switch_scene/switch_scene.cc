#include "switch_scene.h"

using namespace imp;

namespace ix::samsung::homecomponents {

    void SwitchScene::Setup(const std::vector<SceneData>& scenes){
        std::vector<resources::ResourceDefinition> keys;
        for (int i=0;i<scenes.size();i++){
            keys.push_back(scenes[i].scene);
            background_images_.push_back(scenes[i].skybox);
        }
        LoadScene(keys);

        auto skybox_node_ = GetView().CreateNode();
        skybox_component_ = skybox_node_->AddComponent<Skybox>(background_images_);

        auto fade_node_ = GetView().CreateNode();
        fade_component_ = fade_node_->AddComponent<Fade>(1);

        fade_node_->Connect([=](const FadeFinished& fadeFinished){
            if (!fadeFinished.is_fade_in) {
                skybox_component_->UpdateEnvironmentLight(index_);
                DisableAllScenes();
                scenes_[index_]->SetEnabled(true);
                fade_component_->FadeIn();
            }
            else {
                OnSwitchSceneFinished();
            }
        });
    }

    void SwitchScene::LoadScene(std::vector<resources::ResourceDefinition> scene) {
        GetView().GetSceneSystem().LoadScene(scene[loaded_scenes_counter_]).Then([=](NodeHandle node){
            node->SetEnabled(false);
            scenes_[loaded_scenes_counter_] = node;
            auto finishedLoadedEvent = OnSceneFinishedLoading();
            finishedLoadedEvent.loadedNode = node;
            GetNode()->Send(finishedLoadedEvent);
            scene_nodes_[loaded_scenes_counter_] = scenes_[loaded_scenes_counter_];
            loaded_scenes_counter_++;

            if (loaded_scenes_counter_ == 1) {
                scenes_[0]->SetEnabled(true);
            }
            if(loaded_scenes_counter_ > scene.size() - 1) {
                GetNode()->Send(OnScenesLoadFinished());
            } else{
                LoadScene(scene);
            }
        }).KeptBy(this);
    }

    void SwitchScene::DisableAllScenes() {
        for (auto scene : scenes_) {
            scene.second->SetEnabled(false);
        }
    }

    void SwitchScene::SwitchToScene(int index) {
        if (!(index >= 0 and index < scenes_.size()))
            return;

        index_ = index;
        fade_component_->FadeOut();
    }

    void SwitchScene::OnSwitchSceneFinished() {
        GetNode()->Send(SwitchSceneFinished());
    }

    std::unordered_map<int, NodeHandle> SwitchScene::GetLoadedSceneNodes() {
        return scene_nodes_;
    }
}  // namespace ix::samsung::homecomponents
