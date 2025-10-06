using System;

namespace Other
{

  struct ProjectSettings
  {
    public string project_name;
    public string project_path;

    public ProjectSettings()
    {
      project_name = "NewProject";
      project_path = System.IO.Directory.GetCurrentDirectory();
    }

    public ProjectSettings(string name, string path)
    {
      project_name = name;
      project_path = path;
    }
    
    public string GetProjectFilePath()
    {
      return System.IO.Path.Combine(project_path, project_name + ".otherproj");
    }
  }

  public class ProjectBuilder
  {
    public ProjectBuilder()
    {
    }
  }

}