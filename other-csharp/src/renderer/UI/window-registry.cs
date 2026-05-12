using System;
using System.Collections.Generic;
using System.Linq;

namespace Other.UI
{
  public class WindowRegistry
  {
    private static WindowRegistry? instance = null;
    private readonly Dictionary<string, UIWindow> windows = new();

    public WindowRegistry() {
      instance = this;
    }
    ~WindowRegistry() {
      instance = null;
    }

    public static void Register(UIWindow window)
    {
      if (instance!.windows.ContainsKey(window.Title))
      {
        Debug.Warn($"EditorWindow '{window.Title}' is already registered.");
        return;
      }
      instance.windows.Add(window.Title, window);
      window.OnInitialize();
    }

    public static void Register<T>(string title, params object[] args) 
      where T : UIWindow
    {
      if (instance!.windows.ContainsKey(title))
      {
        UIWindow existing_window = instance.windows[title];
        if (existing_window is T)
        {
          Debug.Warn($"EditorWindow '{title}' is already registered.");
        }
        else
        {
          Debug.Error($"A different type of window with the title '{title}' is already registered.");
        }
        return;
      }

      Register((UIWindow)Activator.CreateInstance(typeof(T), args)!);
    }

    public static void Unregister(string title)
    {
      if (instance!.windows.TryGetValue(title, out var window))
      {
        window.OnShutdown();
        instance.windows.Remove(title);
      }
    }

    public static void Unregister<T>()
      where T : UIWindow
    {
      var window_entry = instance!.windows.FirstOrDefault(kvp => kvp.Value is T);
      if (window_entry.Value != null)
      {
        Unregister(window_entry.Key);
      }
    }

    public void RenderAll()
    {
      foreach (var kvp in windows)
      {
        try
        {
          kvp.Value.Render();
        }
        catch (Exception e)
        {
          Debug.Error($"Exception rendering editor window '{kvp.Key}': {e.Message}");
        }
      }
    }

    public static UIWindow GetWindow(string title)
    {
      instance!.windows.TryGetValue(title, out var window);
      return window!;
    }

    public static T GetWindowAs<T>(string title) 
      where T : UIWindow
    {
      UIWindow window = GetWindow(title);
      if (window is not T typed_window)
      {
        throw new InvalidCastException($"Window '{title}' is not of type {typeof(T).Name}.");
      }
      return typed_window;
    }

    public static IEnumerable<string> RegisteredWindowTitles => instance!.windows.Keys;
  }
}