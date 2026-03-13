#ifndef RENDER_PASS_HPP
#define RENDER_PASS_HPP

#include "../../rhi/command_list.hpp"
#include "../render_context.hpp"
#include <glm/glm.hpp>
#include <string>

class Scene;

struct RenderPassData {
  glm::mat4 view;
  glm::mat4 projection;
  glm::vec3 cameraPos;
  int windowWidth;
  int windowHeight;
  RHI::ICommandList *cmdList = nullptr;
};

class RenderPass {
public:
  virtual ~RenderPass() = default;
  virtual bool Initialize(RenderContext *context) = 0;
  virtual void Execute(RenderContext *context, const RenderPassData &data,
                       Scene *scene) = 0;
  virtual std::string GetName() const = 0;
};

#endif
