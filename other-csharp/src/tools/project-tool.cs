using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using OtherCsBindings;

namespace Other.Toolset
{
  class ProjectTool : Tool
  {
    public struct ProjectArgs
    {
      public NativeString Name;
      public NativeString WorkingDirectory;
    }

    public struct Settings
    {
      public string config;
    }

    private string name;
    private string working_directory;
    private string csproj_file_path;

    Process? restore_process_handle = null;
    Process? build_process_handle = null;

    private Settings settings;
    
    public ProjectTool() : base()
    {
      name = string.Empty;
      working_directory = string.Empty;
      csproj_file_path = string.Empty;

      settings = new Settings
      {
        config = "Debug"
      };
    }

    public void SetData(ProjectArgs args)
    {
      name = args.Name.ToString()!;
      working_directory = args.WorkingDirectory.ToString()!;
    }

    public void CreateDefaultCsproj(NativeString path)
    {
      // check if file exists
      // if so error and return
      // if not create a default csproj file with the name and working directory
      string csproj_path = path.ToString()!;

      if (System.IO.File.Exists(csproj_path))
      {
        Console.WriteLine($"Error: File '{csproj_path}' already exists.");
        return;
      }
      
      csproj_file_path = csproj_path;
      string csproj_content = 
      $@"
<Project Sdk=""Microsoft.NET.Sdk"">
  <PropertyGroup>
    <OutputType>Library</OutputType>
    <TargetFramework>net9.0</TargetFramework>
    <RootNamespace>{name}</RootNamespace>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
  </PropertyGroup>
</Project>
";

      System.IO.File.WriteAllText(csproj_file_path, csproj_content);
    }

    public void SetCsprojFilePath(NativeString path)
    {
      csproj_file_path = path.ToString()!;
    }

    public void StartCsprojBuild()
    {
      string csproj_path = csproj_file_path;
      if (!System.IO.File.Exists(csproj_path))
      {
        Console.WriteLine($"Error: File '{csproj_path}' does not exist.");
        return;
      }

      /// check if we need to restore and if we do restore before building
      var restore_process_start_info = new ProcessStartInfo
      {
        FileName = "dotnet.exe",
        Arguments = $"restore \"{csproj_path}\"",
      };

      var build_process_start_info = new ProcessStartInfo
      {
        FileName = "dotnet.exe",
        Arguments = $"build \"{csproj_path}\" -c {settings.config}",
      };


      restore_process_handle = new Process()
      {
        StartInfo = restore_process_start_info
      };
      restore_process_handle.OutputDataReceived += (sender, args) => Debug.Log($"{args.Data}");
      restore_process_handle.ErrorDataReceived += (sender, args) => Debug.Error($"ERROR: {args.Data}");
      restore_process_handle.Exited += (sender, args) => {
        if (restore_process_handle.ExitCode == 0)
        {
          Debug.Log("Restore succeeded, starting build...");
          build_process_handle = new Process()
          {
            StartInfo = build_process_start_info
          };
          build_process_handle.Start();
        }
        else
        {
          Debug.Error($"Restore failed with exit code {restore_process_handle.ExitCode}.");
        }

        restore_process_handle.Dispose();
        restore_process_handle = null;
      };

      
      if (!restore_process_handle.Start())
      {
        Console.WriteLine($"Error: Failed to start restore process for '{csproj_path}'.");
        restore_process_handle = null;
        return;
      }
    }

    public bool IsBuildInProgress()
    {
      if (build_process_handle == null && restore_process_handle == null)
      {
        return false;
      }
      if (restore_process_handle != null && !restore_process_handle.HasExited)
      {
        return true;
      }
      if (build_process_handle != null && !build_process_handle.HasExited)
      {
        return true;
      }
      return false;
    }

    public void CleanupBuild()
    {
      if (build_process_handle != null)
      {
        if (!build_process_handle.HasExited)
        {
          build_process_handle.Kill();
        }
        build_process_handle.Dispose();
        build_process_handle = null;
      }

      if (restore_process_handle != null)
      {
        if (!restore_process_handle.HasExited)
        {
          restore_process_handle.Kill();
        }
        restore_process_handle.Dispose();
        restore_process_handle = null;
      }
    }

    public int GetBuildResult()
    {
      if (build_process_handle == null)
      {
        Console.WriteLine("Error: No build process found.");
        return -1;
      }

      if (!build_process_handle.HasExited)
      {
        Console.WriteLine("Error: Build process is still running.");
        return -1;
      }

      return build_process_handle.ExitCode;
    }
  }
}