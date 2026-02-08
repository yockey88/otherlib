using System;
using System.Collections.Generic;
using System.Numerics;
using OtherCsBindings;

namespace Other.Editor
{
  /// <summary>
  /// Base class for custom windows. Subclass this to create dockable
  /// UI panels in the Other editor, similar to how ui_window works in C++.
  ///
  /// Usage:
  ///   public class MyInspector : UIWindow
  ///   {
  ///     public MyInspector() : base("My Inspector") { }
  ///     protected override void OnRenderBody() { UI.Text("Hello from C#!"); }
  ///   }
  ///
  /// Register windows with UIWindowRegistry.Register() during plugin init.
  /// </summary>
  public abstract class UIWindow
  {
    private string title;
    private bool is_open = true;
    private int window_flags = 0;

    protected UIWindow(string title, int flags = 0)
    {
      this.title = title;
      this.window_flags = flags;
    }

    public string Title
    {
      get => title;
      set => title = value;
    }

    public bool IsOpen
    {
      get => is_open;
      set => is_open = value;
    }

    public int WindowFlags
    {
      get => window_flags;
      set => window_flags = value;
    }

    public void Render()
    {
      if (!is_open) return;

      OnBeforeRender();

      if (UI.BeginWindow(title, window_flags))
      {
        OnRenderHeader();
        OnRenderBody();
        OnRenderFooter();
      }
      UI.EndWindow();

      OnAfterRender();
    }
    protected virtual void OnBeforeRender() { }
    protected virtual void OnRenderHeader() { }
    protected virtual void OnRenderBody() { }
    protected virtual void OnRenderFooter() { }
    protected virtual void OnAfterRender() { }
    public virtual void OnInitialize() { }
    public virtual void OnShutdown() { }
    protected void DrawMenuBar(Action menu_contents)
    {
      if (UI.BeginMenuBar())
      {
        menu_contents?.Invoke();
        UI.EndMenuBar();
      }
    }
    public void Toggle() => is_open = !is_open;
  }

  public abstract class UIWindowWithMenu : UIWindow
  {
    ///  \todo fix this and have enums instead of magic numbers
    // ImGuiWindowFlags_MenuBar = 1 << 10 = 1024
    protected UIWindowWithMenu(string title, int extra_flags = 0)
      : base(title, 1024 | extra_flags)
    {
    }

    protected abstract void OnDrawMenuBar();
    protected override void OnRenderHeader()
    {
      DrawMenuBar(OnDrawMenuBar);
    }
  }

  public static class UIWindowRegistry
  {
    private static readonly Dictionary<string, UIWindow> windows = new();

    public static void Register(UIWindow window)
    {
      if (windows.ContainsKey(window.Title))
      {
        Core.Debug.LogWarning($"EditorWindow '{window.Title}' is already registered.");
        return;
      }
      windows.Add(window.Title, window);
      window.OnInitialize();
    }

    public static void Unregister(string title)
    {
      if (windows.TryGetValue(title, out var window))
      {
        window.OnShutdown();
        windows.Remove(title);
      }
    }

    public static void RenderAll()
    {
      foreach (var kvp in windows)
      {
        try
        {
          kvp.Value.Render();
        }
        catch (Exception e)
        {
          Core.Debug.LogError($"Exception rendering editor window '{kvp.Key}': {e.Message}");
        }
      }
    }

    public static UIWindow GetWindow(string title)
    {
      windows.TryGetValue(title, out var window);
      return window;
    }

    public static T GetWindow<T>(string title) where T : UIWindow
    {
      return GetWindow(title) as T;
    }

    public static IEnumerable<string> RegisteredWindowTitles => windows.Keys;
  }

  [AttributeUsage(AttributeTargets.Class, Inherited = false)]
  public class AutoRegisterEditorWindowAttribute : Attribute
  {
  }
}