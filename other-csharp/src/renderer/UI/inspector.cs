using System;
using System.Numerics;
using System.Reflection;

namespace Other
{
  [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
  public class InspectorFieldAttribute : Attribute
  {
    public string DisplayName { get; set; }
    public string Tooltip { get; set; }
    public float Min { get; set; } = float.MinValue;
    public float Max { get; set; } = float.MaxValue;
    public bool ReadOnly { get; set; } = false;

    public InspectorFieldAttribute(string display_name = null)
    {
      DisplayName = display_name;
    }
  }

  [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
  public class RangeAttribute : Attribute
  {
    public float Min { get; }
    public float Max { get; }

    public RangeAttribute(float min, float max)
    {
      Min = min;
      Max = max;
    }
  }

  [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
  public class ColorFieldAttribute : Attribute
  {
  }

  [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
  public class InspectorGroupAttribute : Attribute
  {
    public string GroupName { get; }

    public InspectorGroupAttribute(string group_name)
    {
      GroupName = group_name;
    }
  }

  [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
  public class InspectorSeparatorAttribute : Attribute
  {
  }

  public static class InspectorDrawer
  {
    public static void DrawInspector(object target, bool draw_all = false)
    {
      if (target == null) {
        return;
      }

      Type type = target.GetType();
      FieldInfo[] fields = type.GetFields(BindingFlags.Public | BindingFlags.Instance);

      string current_group = null;
      bool group_open = false;

      foreach (var field in fields)
      {
        var inspector_attr = field.GetCustomAttribute<InspectorFieldAttribute>();
        if (!draw_all && inspector_attr == null) {
          continue;
        }

        // Handle group headers
        var group_attr = field.GetCustomAttribute<InspectorGroupAttribute>();
        if (group_attr != null && group_attr.GroupName != current_group)
        {
          if (group_open) 
          {
            UI.TreePop();
          }
          current_group = group_attr.GroupName;
          group_open = UI.CollapsingHeader(current_group);
          if (!group_open) 
          {
            continue;
          }
        }
        else if (group_attr == null && current_group != null)
        {
          if (group_open) 
          {
            UI.TreePop();
          }
          current_group = null;
          group_open = false;
        }

        if (current_group != null && !group_open) 
        {
          continue;
        }

        if (field.GetCustomAttribute<InspectorSeparatorAttribute>() != null)
        { 
          UI.Separator();
        }

        string label = inspector_attr?.DisplayName ?? field.Name;
        bool read_only = inspector_attr?.ReadOnly ?? false;

        DrawField(target, field, label, inspector_attr, read_only);
        if (inspector_attr?.Tooltip != null)
        {
          UI.HelpMarker(inspector_attr.Tooltip);
        }
      }

      if (group_open) 
      {
        UI.TreePop();
      }
    }

    private static void DrawField(object target, FieldInfo field, string label, InspectorFieldAttribute attr, bool read_only)
    {
      Type ft = field.FieldType;
      object val = field.GetValue(target);

      if (ft == typeof(float))
      {
        float f = (float)val;
        var range_attr = field.GetCustomAttribute<RangeAttribute>();
        bool changed;
        if (range_attr != null)
        {
          changed = UI.SliderFloat(label, ref f, range_attr.Min, range_attr.Max);
        }
        else
        {
          changed = UI.DragFloat(label, ref f);
        }
        if (changed && !read_only) field.SetValue(target, f);
      }
      else if (ft == typeof(int))
      {
        int i = (int)val;
        var range_attr = field.GetCustomAttribute<RangeAttribute>();
        bool changed;
        if (range_attr != null)
        {
          changed = UI.SliderInt(label, ref i, (int)range_attr.Min, (int)range_attr.Max);
        }
        else
        {
          changed = UI.InputInt(label, ref i);
        }
        if (changed && !read_only)
        {
          field.SetValue(target, i);
        }
      }
      else if (ft == typeof(bool))
      {
        bool b = (bool)val;
        if (UI.Checkbox(label, ref b) && !read_only)
        {
          field.SetValue(target, b);
        }
      }
      else if (ft == typeof(Vec2))
      {
        // Vec2 v = (Vec2)val;
        // bool changed = UI.DragFloat2(label, ref v);
        // if (changed && !read_only)
        // {
        //   field.SetValue(target, v);
        // }
      }
      else if (ft == typeof(Vec3))
      {
        Vec3 v = (Vec3)val;
        bool is_color = field.GetCustomAttribute<ColorFieldAttribute>() != null;
        bool changed;
        if (is_color)
        {
          changed = UI.ColorEdit3(label, ref v);
        }
        else
        {
          changed = UI.DragFloat3(label, ref v);
        }
        if (changed && !read_only)
        {
          field.SetValue(target, v);
        }
      }
      else if (ft == typeof(Vec4))
      {
        Vec4 v = (Vec4)val;
        bool is_color = field.GetCustomAttribute<ColorFieldAttribute>() != null;
        if (is_color)
        {
          if (UI.ColorEdit4(label, ref v) && !read_only)
          {
            field.SetValue(target, v);
          }
        }
        else
        {
          UI.LabelText(label, v.ToString());
        }
      }
      else if (ft == typeof(string))
      {
        string s = (string)val ?? "";
        UI.LabelText(label, s);
      }
      else
      {
        UI.LabelText(label, val?.ToString() ?? "(null)");
      }
    }
  }
}