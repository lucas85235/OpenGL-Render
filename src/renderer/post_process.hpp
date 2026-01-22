#ifndef POST_PROCESS_HPP
#define POST_PROCESS_HPP

#include "../rhi/rhi_device.h"
#include "shader.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class PostProcessEffectType {
  None = 0,
  Grayscale = 1,
  Sepia = 2,
  Invert = 3,
  Vignette = 4,
  ChromaticAberration = 5,
  Sharpen = 6,
  EdgeDetection = 7
};

struct PostProcessSettings {
  PostProcessEffectType effectType = PostProcessEffectType::None;
  float intensity = 1.0f;
  float vignetteRadius = 0.75f;
  float vignetteStrength = 0.5f;
  float chromaticOffset = 0.005f;
  float sharpenStrength = 1.0f;
};

class PostProcessEffect {
public:
  virtual ~PostProcessEffect() = default;
  virtual PostProcessEffectType GetType() const = 0;
  virtual const char *GetName() const = 0;
  virtual void ApplyUniforms(Shader &shader) = 0;

  bool IsEnabled() const { return enabled; }
  void SetEnabled(bool e) { enabled = e; }
  float GetIntensity() const { return intensity; }
  void SetIntensity(float i) { intensity = i; }

protected:
  bool enabled = true;
  float intensity = 1.0f;
};

class GrayscaleEffect : public PostProcessEffect {
public:
  PostProcessEffectType GetType() const override {
    return PostProcessEffectType::Grayscale;
  }
  const char *GetName() const override { return "Grayscale"; }
  void ApplyUniforms(Shader &shader) override {
    shader.SetInt("u_effectType", static_cast<int>(GetType()));
    shader.SetFloat("u_intensity", intensity);
  }
};

class SepiaEffect : public PostProcessEffect {
public:
  PostProcessEffectType GetType() const override {
    return PostProcessEffectType::Sepia;
  }
  const char *GetName() const override { return "Sepia"; }
  void ApplyUniforms(Shader &shader) override {
    shader.SetInt("u_effectType", static_cast<int>(GetType()));
    shader.SetFloat("u_intensity", intensity);
  }
};

class InvertEffect : public PostProcessEffect {
public:
  PostProcessEffectType GetType() const override {
    return PostProcessEffectType::Invert;
  }
  const char *GetName() const override { return "Invert"; }
  void ApplyUniforms(Shader &shader) override {
    shader.SetInt("u_effectType", static_cast<int>(GetType()));
    shader.SetFloat("u_intensity", intensity);
  }
};

class VignetteEffect : public PostProcessEffect {
public:
  float radius = 0.75f;
  float strength = 0.5f;

  PostProcessEffectType GetType() const override {
    return PostProcessEffectType::Vignette;
  }
  const char *GetName() const override { return "Vignette"; }
  void ApplyUniforms(Shader &shader) override {
    shader.SetInt("u_effectType", static_cast<int>(GetType()));
    shader.SetFloat("u_intensity", intensity);
    shader.SetFloat("u_vignetteRadius", radius);
    shader.SetFloat("u_vignetteStrength", strength);
  }
};

class ChromaticAberrationEffect : public PostProcessEffect {
public:
  float offset = 0.005f;

  PostProcessEffectType GetType() const override {
    return PostProcessEffectType::ChromaticAberration;
  }
  const char *GetName() const override { return "ChromaticAberration"; }
  void ApplyUniforms(Shader &shader) override {
    shader.SetInt("u_effectType", static_cast<int>(GetType()));
    shader.SetFloat("u_intensity", intensity);
    shader.SetFloat("u_chromaticOffset", offset);
  }
};

class SharpenEffect : public PostProcessEffect {
public:
  float strength = 1.0f;

  PostProcessEffectType GetType() const override {
    return PostProcessEffectType::Sharpen;
  }
  const char *GetName() const override { return "Sharpen"; }
  void ApplyUniforms(Shader &shader) override {
    shader.SetInt("u_effectType", static_cast<int>(GetType()));
    shader.SetFloat("u_intensity", intensity);
    shader.SetFloat("u_sharpenStrength", strength);
  }
};

class EdgeDetectionEffect : public PostProcessEffect {
public:
  PostProcessEffectType GetType() const override {
    return PostProcessEffectType::EdgeDetection;
  }
  const char *GetName() const override { return "EdgeDetection"; }
  void ApplyUniforms(Shader &shader) override {
    shader.SetInt("u_effectType", static_cast<int>(GetType()));
    shader.SetFloat("u_intensity", intensity);
  }
};

class PostProcessStack {
private:
  std::vector<std::unique_ptr<PostProcessEffect>> effects;
  int activeEffectIndex = -1;

public:
  template <typename T> T *AddEffect() {
    auto effect = std::make_unique<T>();
    T *ptr = effect.get();
    effects.push_back(std::move(effect));
    return ptr;
  }

  void RemoveEffect(PostProcessEffectType type) {
    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
                       [type](const auto &e) { return e->GetType() == type; }),
        effects.end());
  }

  PostProcessEffect *GetEffect(PostProcessEffectType type) {
    for (auto &e : effects) {
      if (e->GetType() == type)
        return e.get();
    }
    return nullptr;
  }

  void SetActiveEffect(int index) {
    if (index >= -1 && index < static_cast<int>(effects.size())) {
      activeEffectIndex = index;
    }
  }

  void CycleEffect() {
    activeEffectIndex++;
    if (activeEffectIndex >= static_cast<int>(effects.size())) {
      activeEffectIndex = -1; // Disable all
    }
  }

  PostProcessEffect *GetActiveEffect() {
    if (activeEffectIndex >= 0 &&
        activeEffectIndex < static_cast<int>(effects.size())) {
      return effects[activeEffectIndex].get();
    }
    return nullptr;
  }

  int GetActiveIndex() const { return activeEffectIndex; }
  size_t GetEffectCount() const { return effects.size(); }

  void ApplyToShader(Shader &shader) {
    PostProcessEffect *active = GetActiveEffect();
    if (active && active->IsEnabled()) {
      active->ApplyUniforms(shader);
    } else {
      shader.SetInt("u_effectType", 0); // None
    }
  }

  void Clear() {
    effects.clear();
    activeEffectIndex = -1;
  }
};

#endif // POST_PROCESS_HPP
