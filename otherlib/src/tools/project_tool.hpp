/**
 * \file tools/project_tool.hpp
 **/
#ifndef OTHERLIB_TOOLS_PROJECT_TOOL_HPP
#define OTHERLIB_TOOLS_PROJECT_TOOL_HPP

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
    inline std::vector<filepath> get_collected_cs_script_files() const { return cs_script_files; }

   private:
    filepath dotnet_project_path;

    std::vector<filepath> cs_script_files;
    /// \todo should we do this for lua?

    void collect_cs_script_files(const filepath& directory);
  };

}  // namespace other

#endif  // OTHERLIB_TOOLS_PROJECT_TOOL_HPP