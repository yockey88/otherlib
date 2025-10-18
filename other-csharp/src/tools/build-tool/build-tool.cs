using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using OtherCsBindings;

namespace Other
{ 
  public class BuildTool //: Toolset.Tool
  {
    enum BuildPhase
    {
      VALIDATE_GENERATE_FILES = 0,
      PRE_BUILD_STEPS,
      BUILD,
      POST_BUILD_STEPS,
      CLEANUP,

      NUM_BUILD_PHASES,
      INVALID_BUILD_PHASE = BuildPhase.NUM_BUILD_PHASES,
    };

    public struct BuildArgs
    {
      public NativeString project_type;
      public NativeString Name;
      public NativeString FileName;
      public NativeString WorkingDirectory;
    }

    ProjectDescription project;

    struct FileData
    {
      public string template_path;
      public string destination_path;
      public override string ToString()
      {
        return $"Template: {template_path}, Destination: {destination_path}";
      }
    }

    public BuildTool()
    {
      project = new ProjectDescription();
    }

    private Dictionary<string, FileData> GetFileDataFromTemplates(string proj_working_directory, string project_name)
    {
      Dictionary<string, FileData> project_templates = new Dictionary<string, FileData>();

      string templates_dir = Path.Combine(Core.Filesystem.GetInstallFolder(), "templates");
      foreach (string path in Directory.GetFiles(templates_dir))
      {
        /// replace each template file name with corresponding destination file path as described here:
        ///    1- .cpp/.hpp -> src/<project-name>.cpp/hpp
        ///    2- CMakeLists1.txt -> CMakeLists.txt
        ///    3- CMakeLists2.txt -> src/CMakeLists.txt
        ///    4- .toml -> <project-name>.toml
        string filename_no_ext = Path.GetFileNameWithoutExtension(path);
        string filename = Path.GetFileName(path);
        string ext = Path.GetExtension(path);

        string destination_path = "";
        if (ext == ".cpp" || ext == ".hpp")
        {
          if (filename_no_ext.EndsWith("-driver"))
          {
            destination_path = Path.Combine(proj_working_directory, "src", GetFileVersionOfProjectName(project_name) + "_driver" + ext);
          }
          else
          {
            destination_path = Path.Combine(proj_working_directory, "src", GetFileVersionOfProjectName(project_name) + ext);
          }
        }
        else if (filename == "CMakeLists1.txt")
        {
          destination_path = Path.Combine(proj_working_directory, "CMakeLists.txt");
        }
        else if (filename == "CMakeLists2.txt")
        {
          destination_path = Path.Combine(proj_working_directory, "src", "CMakeLists.txt");
        }
        else if (ext == ".toml")
        {
          destination_path = Path.Combine(proj_working_directory, GetFileVersionOfProjectName(project_name) + ext);
        }
        else
        {
          Core.Debug.LogWarning($"Unknown template file '{filename}', skipping.");
          continue;
        }

        project_templates[filename] = new FileData()
        {
          template_path = path,
          destination_path = destination_path,
        };
      }

      return project_templates;
    }

    public void CreateProject(BuildArgs args)
    {
      try
      {
        string proj_pwd = args.WorkingDirectory;
        if (!Directory.Exists(proj_pwd))
        {
          Core.Debug.LogError($"Working directory '{proj_pwd}' does not exist, cannot create project.");
          return;
        }

        project = new ProjectDescription()
        {
          project_type = GetProjectTypeFromBuildArgs(args),
          name = args.Name,
          filename = args.FileName,
          working_dir = args.WorkingDirectory,
        };
        project.working_dir.Replace("\\", "/");
        project.filename.Replace("\\", "/");

        string src_dir = Path.Combine(proj_pwd, "src");
        if (!Directory.Exists(src_dir))
        {
          Directory.CreateDirectory(src_dir);
        }

        string asset_dir = Path.Combine(proj_pwd, "assets");
        if (!Directory.Exists(asset_dir))
        {
          Directory.CreateDirectory(asset_dir);
        }

        var project_templates = GetFileDataFromTemplates(proj_pwd, args.Name.ToString());
        foreach (var file_data in project_templates.Values)
        {
          try
          {
            WriteTemplatedFile(file_data.template_path, file_data.destination_path);
          }
          catch (Exception e)
          {
            Core.Debug.LogError($"Failed to generate file from template '{file_data.template_path}': {e.Message}");
          }
        }

        using (var process = new Process())
        {
          process.StartInfo.FileName = "cmake.exe";
          process.StartInfo.Arguments = $"-S {project.working_dir} -B {project.working_dir}/build -G \"Visual Studio 17 2022\"";
          process.StartInfo.RedirectStandardOutput = true;
          process.StartInfo.RedirectStandardError = true;
          process.StartInfo.UseShellExecute = false;
          process.StartInfo.CreateNoWindow = true;
          process.OutputDataReceived += (sender, e) =>
          {
            if (!string.IsNullOrEmpty(e.Data))
            {
              Core.Debug.Log(e.Data);
            }
          };
          process.ErrorDataReceived += (sender, e) =>
          {
            if (!string.IsNullOrEmpty(e.Data))
            {
              Core.Debug.LogError(e.Data);
            }
          };
          process.Start();
        }
      }
      catch (Exception e)
      {
        Core.Debug.LogError($"Exception while creating project: {e.Message}");
      }
    }

    public void StartBuild(BuildArgs project_description)
    {
      project_description.FileName = GetFileNameFromBuildArgs(project_description);
      if (!ValidateBuildArgs(project_description))
      {
        Core.Debug.LogError("Build arguments are invalid, aborting build.");
        return;
      }

      project = new ProjectDescription()
      {
        project_type = GetProjectTypeFromBuildArgs(project_description),
        name = project_description.Name,
        filename = project_description.FileName,
        working_dir = project_description.WorkingDirectory,
      };

      Log($"Validating project '{project_description.Name}' in '{project_description.WorkingDirectory}' with file '{project_description.FileName}'.");
      Log($"Project type: {project_description.project_type}");

      if (!ValidateProject())
      {
        Core.Debug.LogError("Project validation failed, aborting build.");
        return;
      }
    }


    private bool ValidateProject()
    {
      /// check if required files are present
      var required_paths = GetRequiredPathsForBuild(project.name, project.working_dir);
      foreach (var path in required_paths)
      {
        string ext = Path.GetExtension(path);

        /// folder
        if (string.IsNullOrEmpty(ext) || path == Path.Combine(project.working_dir, ".other"))
        {
          if (!Directory.Exists(path))
          {
            try 
            {
              Directory.CreateDirectory(path);
            }
            catch (Exception e)
            {
              Core.Debug.LogError($"Failed to create missing directory '{path}': {e.Message}");
              return false;
            }
          }
        }
        else
        {
          if (!File.Exists(path))
          {
            /// \todo this is disabled for now, we should not auto-generate files yet
            // if (!AttemptGeneration(path))
            // {
            //   Core.Debug.LogError($"Failed to generate missing file '{path}'.");
            //   return false;
            // }
            return false;
          }
        }
      }

      return true;
    }

    private void WriteTemplatedFile(string template_path, string destination_path)
    {
      if (!File.Exists(template_path))
      {
        Core.Debug.LogError($"Template file '{template_path}' does not exist.");
        return;
      }

      if (File.Exists(destination_path))
      {
        Core.Debug.LogWarning($"Destination file '{destination_path}' already exists, Overwriting.");
      }
      
      string content = File.ReadAllText(template_path);
      content = content.Replace("${project-name}", project.name)
                        .Replace("${project-name-upper}", project.name.ToUpper())
                        .Replace("${project-folder}", project.working_dir)
                        .Replace("${environment-file}", GetFileVersionOfProjectName(project.name) + ".toml")
                        // \todo fix this to be the actual install dir
                        .Replace("${otherlib-install-dir}", Core.Filesystem.GetInstallFolder())
                        .Replace("\\", "/");

      File.WriteAllText(destination_path, content);
    }

    public List<string> GetRequiredPathsForBuild(NativeString name, NativeString working_dir)
    {
      /// replace all ${*} variables in ExpectedProjectPaths with actual values
      var required_paths = new List<string>();

      foreach (var path in ProjectConfig.ExpectedProjectPaths)
      {
        var replaced_path = path.Replace("${project-folder}", working_dir)
                                .Replace("${project-name}", GetFileVersionOfProjectName(name))
                                .Replace("${environment-file}", GetFileVersionOfProjectName(name) + ".toml");
        required_paths.Add(replaced_path);
      }

      return required_paths;
    }

    private string GetFileVersionOfProjectName(string project_name)
    {
      string expected_ext = ".toml";
      string ext = Path.GetExtension(project_name);
      if (ext == expected_ext)
      {
        project_name = Path.GetFileNameWithoutExtension(project_name);
      }
      else if (!string.IsNullOrEmpty(ext))
      {
        Core.Debug.LogWarning($"Project name has unexpected extension '{ext}', expected '{expected_ext}'. Ignoring possible extension.");
      }

      /// lower-case, spaces to hyphens, alphanumeric and hyphens only
      var file_version = project_name.ToLower().Replace(" ", "-").Where(c => char.IsLetterOrDigit(c) || c == '-').Aggregate("", (s, c) => s + c);
      return file_version;
    }

    private bool ValidateBuildArgs(BuildArgs args)
    {
      if (string.IsNullOrEmpty(args.Name))
      {
        Core.Debug.LogError("Project name is empty.");
        return false;
      }

      if (string.IsNullOrEmpty(args.WorkingDirectory))
      {
        Core.Debug.LogError("Working directory is empty.");
        return false;
      }

      if (!Directory.Exists(args.WorkingDirectory))
      {
        Core.Debug.LogError($"Working directory '{args.WorkingDirectory}' does not exist.");
        return false;
      }

      if (string.IsNullOrEmpty(args.FileName))
      {
        Core.Debug.LogError("Project file name is empty.");
        return false;
      }

      return true;
    }

    private NativeString GetFileNameFromBuildArgs(BuildArgs args)
    {
      NativeString file_name = args.FileName;
      if (string.IsNullOrEmpty(file_name))
      {
        /// check if it is a full project file or just a name w/o extension
        string ext = Path.GetExtension(args.Name);
        if (!string.IsNullOrEmpty(ext) && ext != ".toml")
        {
          Core.Debug.LogWarning($"Project name has unexpected extension '{ext}', expected '.toml'. Ignoring possible extension and renaming file.");
        }
        else
        {
          Log("No project file specified, generating from project name.");
        }
        file_name = GetFileVersionOfProjectName(args.Name);
      }
      else
      {
        string ext = Path.GetExtension(file_name);
        if (ext != ".toml")
        {
          Core.Debug.LogWarning($"Project file has unexpected extension '{ext}', expected '.toml'. Ignoring possible extension and renaming file.");
          file_name = GetFileVersionOfProjectName(file_name);
        }
        else
        {
          Log($"Using specified project file name: {file_name}.");
        }
      }

      return file_name;
    }

    private ProjectConfig.ProjectType GetProjectTypeFromBuildArgs(BuildArgs args)
    {
      if (args.project_type == "Application")
      {
        return ProjectConfig.ProjectType.APPLICATION;
      }
      else if (args.project_type == "Module")
      {
        return ProjectConfig.ProjectType.MODULE;
      }
      else
      {
        Core.Debug.LogWarning($"Unknown project type '{args.project_type}', defaulting to 'Application'.");
        return ProjectConfig.ProjectType.APPLICATION;
      }
    }

    private void Log(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      Core.Debug.Log(message, Logger.LogLevel.Debug, memberName, lineNumber);
    }

    private struct ProjectDescription
    {
      public ProjectConfig.ProjectType project_type;
      public string name;
      public string filename;
      public string working_dir;
    }
  }
}