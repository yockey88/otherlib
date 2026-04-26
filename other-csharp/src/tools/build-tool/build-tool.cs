using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using OtherCsBindings;

namespace Other
{ 
  public class BuildTool : Toolset.Tool
  {
    enum BuildPhase
    {
      VALIDATE_GENERATE_FILES = 0,
      PRE_BUILD_STEPS,
      BUILD,
      POST_BUILD_STEPS,
      CLEANUP,

      NUM_BUILD_PHASES,
      INVALID_BUILD_PHASE = NUM_BUILD_PHASES,
    };

    enum BuildStatus
    {
      BUILD_STATUS_NOT_STARTED = 0,
      GENERATING_BUILD_SYSTEM,
      CURRENTLY_BUILDING,
      BUILD_STATUS_SUCCESS,
      BUILD_STATUS_FAILED,

      NUM_BUILD_STATUSES,
      INVALID_BUILD_STATUS = NUM_BUILD_STATUSES,
    };

    public struct ProjectCreationArgs
    {
      public NativeString project_type;
      public NativeString Name;
      public NativeString FileName;
      public NativeString WorkingDirectory;
    }

    struct FileData
    {
      public string template_path;
      public string destination_path;
      public override string ToString()
      {
        return $"Template: {template_path}, Destination: {destination_path}";
      }
    }

    ProjectDescription project;
    BuildStatus current_build_status = BuildStatus.BUILD_STATUS_NOT_STARTED;

#nullable enable
    Process? process_handle = null;
#nullable disable

    public BuildTool()
    {
      project = new ProjectDescription();
    }

    public Int32 GetBuildStatus()
    {
      return (Int32)current_build_status;
    }

    private Dictionary<string, FileData> GetFileDataFromTemplates(string proj_working_directory, string project_name)
    {
      Dictionary<string, FileData> project_templates = new Dictionary<string, FileData>();

      string templates_dir = Path.Combine(Core.Filesystem.GetInstallFolder(), "project/templates");
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

        string expected_cmake_filename = project.project_type == ProjectConfig.ProjectType.APPLICATION ?
          "CMakeLists.application.txt" : "CMakeLists.module.txt";

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
        else if (filename == expected_cmake_filename)
        {
          destination_path = Path.Combine(proj_working_directory, "CMakeLists.txt");
        }
        else if (ext == ".toml")
        {
          destination_path = Path.Combine(proj_working_directory, GetFileVersionOfProjectName(project_name) + ext);
        }
        else
        {
          Debug.Warn($"Unknown template file '{filename}', skipping.");
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

    private void GenerateSolution()
    {
      process_handle = new Process();
      process_handle.StartInfo.FileName = "cmake.exe";
      process_handle.StartInfo.Arguments = $"-S {project.working_dir} -B {project.working_dir}/build -G \"Visual Studio 17 2022\"";
      // process_handle.StartInfo.RedirectStandardOutput = false;  // true;
      // process_handle.StartInfo.RedirectStandardError = false;   // true;
      // process_handle.StartInfo.UseShellExecute = false;
      // process_handle.StartInfo.CreateNoWindow = true;
      // process_handle.OutputDataReceived += (sender, e) =>
      // {
      //   if (!string.IsNullOrEmpty(e.Data))
      //   {
      //     Core.Debug.Log(e.Data);
      //   }
      // };
      // process_handle.ErrorDataReceived += (sender, e) =>
      // {
      //   if (!string.IsNullOrEmpty(e.Data))
      //   {
      //     Core.Debug.LogError(e.Data);
      //   }
      // };
      Debug.Log($"Generating build solution in '{project.working_dir}/build'...");

      current_build_status = BuildStatus.GENERATING_BUILD_SYSTEM;
      process_handle.Start();
    }

    private void BuildSolution()
    {
      process_handle = new Process();
      process_handle.StartInfo.FileName = "cmake.exe";
      /// \todo make config selectable
      process_handle.StartInfo.Arguments = $"--build {project.working_dir}/build --config Debug";
      // process_handle.StartInfo.RedirectStandardOutput = false;  // true;
      // process_handle.StartInfo.RedirectStandardError = false;   // true;
      // process_handle.StartInfo.UseShellExecute = false;
      // process_handle.StartInfo.CreateNoWindow = true;
      // process_handle.OutputDataReceived += (sender, e) =>
      // {
      //   if (!string.IsNullOrEmpty(e.Data))
      //   {
      //     Core.Debug.Log(e.Data);
      //   }
      // };
      // process_handle.ErrorDataReceived += (sender, e) =>
      // {
      //   if (!string.IsNullOrEmpty(e.Data))
      //   {
      //     Core.Debug.LogError(e.Data);
      //   }
      // };
      Debug.Log($"Building project solution in '{project.working_dir}/build'...");

      current_build_status = BuildStatus.CURRENTLY_BUILDING;
      process_handle.Start();
    }

    public void CreateProject(ProjectCreationArgs args)
    {
      try
      {
        string proj_pwd = args.WorkingDirectory;
        if (!Directory.Exists(proj_pwd))
        {
          Debug.Error($"Working directory '{proj_pwd}' does not exist, cannot create project.");
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
            Debug.Error($"Failed to generate file from template '{file_data.template_path}': {e.Message}");
          }
        }

        BeginGenerateAndBuild();
      }
      catch (Exception e)
      {
        Debug.Error($"Exception while creating project: {e.Message}");
      }
    }

    private void BeginGenerateAndBuild()
    {
      GenerateSolution();
    }

    public Int32 PollProjectBuild()
    {
      if (process_handle == null ||
          current_build_status == BuildStatus.BUILD_STATUS_SUCCESS ||
          current_build_status == BuildStatus.BUILD_STATUS_FAILED)
      {
        return (Int32)current_build_status;
      }

      if (process_handle.HasExited)
      {
        if (current_build_status == BuildStatus.GENERATING_BUILD_SYSTEM)
        {
          if (process_handle.ExitCode == 0)
          {
            process_handle = null;

            Debug.Log("Build system generation completed successfully.");
            BuildSolution();
          }
          else
          {
            Debug.Error($"Build system generation failed with exit code {process_handle.ExitCode}.");
            current_build_status = BuildStatus.BUILD_STATUS_FAILED;
          }
        }
        else if (current_build_status == BuildStatus.CURRENTLY_BUILDING)
        {
          if (process_handle.ExitCode == 0)
          {
            process_handle = null;

            Debug.Log("Project build completed successfully.");
            current_build_status = BuildStatus.BUILD_STATUS_SUCCESS;
          }
          else
          {
            Debug.Error($"Project build failed with exit code {process_handle.ExitCode}.");
            current_build_status = BuildStatus.BUILD_STATUS_FAILED;
          }
        }

        return (Int32)current_build_status;
      }
      else
      {
        return (Int32)current_build_status;  
      }
    }

    public void FinalizeBuild()
    {
      if (process_handle != null && !process_handle.HasExited)
      {
        process_handle.Kill();
        process_handle = null;
      }

      Debug.Log("Finalized build process.");
    }

    private void WriteTemplatedFile(string template_path, string destination_path)
    {
      if (!File.Exists(template_path))
      {
        Debug.Error($"Template file '{template_path}' does not exist.");
        return;
      }

      if (File.Exists(destination_path))
      {
        Debug.Warn($"Destination file '{destination_path}' already exists, Overwriting.");
      }

      Debug.Log($"Generating file '{destination_path}' from template '{template_path}'.");
      
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
        Debug.Warn($"Project name has unexpected extension '{ext}', expected '{expected_ext}'. Ignoring possible extension.");
      }

      /// lower-case, spaces to hyphens, alphanumeric and hyphens only
      var file_version = project_name.ToLower().Replace(" ", "-").Where(c => char.IsLetterOrDigit(c) || c == '-').Aggregate("", (s, c) => s + c);
      return file_version;
    }

    private bool ValidateBuildArgs(ProjectCreationArgs args)
    {
      if (string.IsNullOrEmpty(args.Name))
      {
        Debug.Error("Project name is empty.");
        return false;
      }

      if (string.IsNullOrEmpty(args.WorkingDirectory))
      {
        Debug.Error("Working directory is empty.");
        return false;
      }

      if (!Directory.Exists(args.WorkingDirectory))
      {
        Debug.Error($"Working directory '{args.WorkingDirectory}' does not exist.");
        return false;
      }

      if (string.IsNullOrEmpty(args.FileName))
      {
        Debug.Error("Project file name is empty.");
        return false;
      }

      return true;
    }

    private NativeString GetFileNameFromBuildArgs(ProjectCreationArgs args)
    {
      NativeString file_name = args.FileName;
      if (string.IsNullOrEmpty(file_name))
      {
        /// check if it is a full project file or just a name w/o extension
        string ext = Path.GetExtension(args.Name);
        if (!string.IsNullOrEmpty(ext) && ext != ".toml")
        {
          Debug.Warn($"Project name has unexpected extension '{ext}', expected '.toml'. Ignoring possible extension and renaming file.");
        }
        else
        {
          SendLog("No project file specified, generating from project name.");
        }
        file_name = GetFileVersionOfProjectName(args.Name);
      }
      else
      {
        string ext = Path.GetExtension(file_name);
        if (ext != ".toml")
        {
          Debug.Warn($"Project file has unexpected extension '{ext}', expected '.toml'. Ignoring possible extension and renaming file.");
          file_name = GetFileVersionOfProjectName(file_name);
        }
        else
        {
          SendLog($"Using specified project file name: {file_name}.");
        }
      }

      return file_name;
    }

    private ProjectConfig.ProjectType GetProjectTypeFromBuildArgs(ProjectCreationArgs args)
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
        Debug.Warn($"Unknown project type '{args.project_type}', defaulting to 'Application'.");
        return ProjectConfig.ProjectType.APPLICATION;
      }
    }

    private void SendLog(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      Debug.Log(message, memberName, lineNumber);
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