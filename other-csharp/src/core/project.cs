using System;

namespace Other
{
  public class Project
  {
    public enum Type : UInt32
    {
      /// will build a headless Other Environment application
      /// useful for server/backend or command-line applications
      CONSOLE_APPLICATION,
      /// will build a desktop application with a window and rendering capabilities
      /// useful for games or simulations
      RENDERING_APPLICATION,
      /// will build a plugin library that can be loaded into an Other Environment application
      /// useful for extending the functionality of an existing application or for creating reusable components
      /// note: plugin projects will not be executable on their own and require an existing application to load them
      PLUGIN_LIBRARY,
    }

    private Type type;

    public Type ProjectType
    {
      get => type;
    }

    public Project(Type type)
    {
      this.type = type;
    }
  }
}