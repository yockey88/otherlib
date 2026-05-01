using System;

namespace Other.UI
{
  public abstract class UIWindow
  {
    private string title;
    private bool is_open = true;
    private int window_flags = 0;

    protected UIWindow(string title, int flags = 0)
    {
      this.title = title;
      this.window_flags = flags;

      WindowRegistry.Register(this);
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
      if (!is_open) 
      {
        return;
      }

      OnBeforeRender();

      if (UIBindings.BeginWindow(title, window_flags))
      {
        OnRenderHeader();
        OnRenderBody();
        OnRenderFooter();
      }
      UIBindings.EndWindow();

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
      if (UIBindings.BeginMenuBar())
      {
        menu_contents?.Invoke();
        UIBindings.EndMenuBar();
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
}