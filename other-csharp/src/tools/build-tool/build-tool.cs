using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using Other.Core;
using OtherCsBindings;

namespace Other
{ 
  public class BuildTool //: Toolset.Tool
  {
    enum BuildPhase {
      VALIDATE_GENERATE_FILES = 0,
      PRE_BUILD_STEPS,
      BUILD,
      POST_BUILD_STEPS,
      CLEANUP,

      NUM_BUILD_PHASES,
      INVALID_BUILD_PHASE = BuildPhase.NUM_BUILD_PHASES,
    };

    public BuildTool()
    {
      project = new ProjectDescription();
    }

    public struct BuildArgs
    {
      public NativeString project_type;
      public NativeString Name;
      public NativeString FileName;
      public NativeString WorkingDirectory;
    }

    private struct ProjectDescription
    {
      public ProjectConfig.ProjectType project_type;
      public string name;
      public string filename;
      public string working_dir;
    }

    ProjectDescription project;

    struct FileData
    {
      public string template_path;
      public string destination_path;
    }

    Queue<string> files_to_generate = new Queue<string>();
    Dictionary<string, FileData> project_templates = new Dictionary<string, FileData>();

    public void StartBuild(BuildArgs project_description)
    {
      project_description.FileName = GetFileNameFromBuildArgs(project_description);
      if (!ValidateBuildArgs(project_description))
      {
        Debug.LogError("Build arguments are invalid, aborting build.");
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
        Debug.LogError("Project validation failed, aborting build.");
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
              Debug.LogError($"Failed to create missing directory '{path}': {e.Message}");
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
            //   Debug.LogError($"Failed to generate missing file '{path}'.");
            //   return false;
            // }
            return false;
          }
        }
      }

      return true;
    }

    private bool AttemptGeneration(string file)
    {
      try
      {
        string ext = Path.GetExtension(file);
        if (string.IsNullOrEmpty(ext))
        {
          Debug.LogError($"Cannot generate directory '{file}', please create it manually.");
          return false;
        }

        string template_path = "";
        string destination_path = Path.Combine(project.working_dir, Path.GetFileName(file));

        /// special case is build files, the two CMakeLists.txt files
        if (ext == ".txt" && Path.GetFileName(file) == "CMakeLists.txt")
        {
          /// check if src or top level CMake
          bool top_level = Path.GetDirectoryName(file) == project.working_dir;
          string root_template_path = "templates/CMakeLists1.txt";
          string src_template_path = "templates/CMakeLists2.txt";
          template_path = top_level ? root_template_path : src_template_path;

          return true;
        }
        else
        {
          if (!ProjectConfig.TemplatePaths.ContainsKey(ext))
          {
            Debug.LogError($"No template defined for files with extension '{ext}', cannot generate '{file}'.");
            return false;
          }

          template_path = ProjectConfig.TemplatePaths[ext];
        }

        if (string.IsNullOrEmpty(template_path))
        {
          Debug.LogError($"No template found for generating file '{file}'.");
          return false;
        }

        WriteTemplatedFile(template_path, destination_path);
        return true;
      }
      catch (Exception e)
      {
        Debug.LogError($"Exception while generating file '{file}': {e.Message}");
        return false;
      }
    }

    private void WriteTemplatedFile(string template_path, string destination_path)
    {
      try
      {
        if (!File.Exists(template_path))
        {
          Debug.LogError($"Template file '{template_path}' does not exist.");
          return;
        }

        string content = File.ReadAllText(template_path);
        content = content.Replace("${project-name}", project.name)
                         .Replace("${project-folder}", project.working_dir)
                         .Replace("${environment-file}", GetFileVersionOfProjectName(project.name) + ".toml")
                         // \todo fix this to be the actual install dir
                         .Replace("${other-install-dir}", "C:/Yock/code/Other2/OtherEnv");

        // File.WriteAllText(destination_path, content);
        Log($"Generated file '{destination_path}' from template '{template_path}'.");
      }
      catch (Exception e)
      {
        Debug.LogError($"Exception while writing templated file '{destination_path}': {e.Message}");
      }
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
        Debug.LogWarning($"Project name has unexpected extension '{ext}', expected '{expected_ext}'. Ignoring possible extension.");
      }

      /// lower-case, spaces to hyphens, alphanumeric and hyphens only
      var file_version = project_name.ToLower().Replace(" ", "-").Where(c => char.IsLetterOrDigit(c) || c == '-').Aggregate("", (s, c) => s + c);
      return file_version;
    }

    private bool ValidateBuildArgs(BuildArgs args)
    {
      if (string.IsNullOrEmpty(args.Name))
      {
        Debug.LogError("Project name is empty.");
        return false;
      }

      if (string.IsNullOrEmpty(args.WorkingDirectory))
      {
        Debug.LogError("Working directory is empty.");
        return false;
      }

      if (!Directory.Exists(args.WorkingDirectory))
      {
        Debug.LogError($"Working directory '{args.WorkingDirectory}' does not exist.");
        return false;
      }

      if (string.IsNullOrEmpty(args.FileName))
      {
        Debug.LogError("Project file name is empty.");
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
          Debug.LogWarning($"Project name has unexpected extension '{ext}', expected '.toml'. Ignoring possible extension and renaming file.");
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
          Debug.LogWarning($"Project file has unexpected extension '{ext}', expected '.toml'. Ignoring possible extension and renaming file.");
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
        Debug.LogWarning($"Unknown project type '{args.project_type}', defaulting to 'Application'.");
        return ProjectConfig.ProjectType.APPLICATION;
      }
    }

    private void Log(string message, [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0)
    {
      Debug.Log(message, Logger.LogLevel.Debug, memberName, lineNumber);
    }
  }

}