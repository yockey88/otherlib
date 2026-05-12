/**
 * \file tools/project_tool.hpp
 **/
#ifndef OTHER_SCENE_TOOLS_PROJECT_TOOL_HPP
#define OTHER_SCENE_TOOLS_PROJECT_TOOL_HPP

#include "core/coroutine.hpp"

#include "tools/tool.hpp"

namespace other {

  class project_tool : public tool {
   public:
    project_tool()
        : tool("Other.Toolset.ProjectTool") {}
    ~project_tool() = default;

    void generate_dotnet_project(const filepath& csproj_path);

    void start_project_build(const filepath& csproj_path);
    bool project_build_in_progress() const;
    void cleanup_build();
    int32_t get_build_result() const;

    const filepath& get_dotnet_project_path() const { return dotnet_project_path; }

   private:
    filepath dotnet_project_path;
  };

}  // namespace other

#endif  // OTHER_SCENE_TOOLS_PROJECT_TOOL_HPP