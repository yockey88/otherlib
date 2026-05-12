using OtherCsBindings;
using Other.Core;
using System;

namespace Other
{
  public static class UIBindings
  {
    [NativeFunction("BeginWindow")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginWindow;

    [NativeFunction("EndWindow")]
    internal static unsafe delegate*<void> NativeEndWindow;

    [NativeFunction("BeginChild")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginChild;
    [NativeFunction("EndChild")]
    internal static unsafe delegate*<void> NativeEndChild;
    [NativeFunction("UIText")]
    internal static unsafe delegate*<NativeString, void> NativeText;
    [NativeFunction("UITextColored")]
    internal static unsafe delegate*<float, float, float, float, NativeString, void> NativeTextColored;
    [NativeFunction("UITextWrapped")]
    internal static unsafe delegate*<NativeString, void> NativeTextWrapped;
    [NativeFunction("UILabelText")]
    internal static unsafe delegate*<NativeString, NativeString, void> NativeLabelText;
    [NativeFunction("UIButton")]
    internal static unsafe delegate*<NativeString, float, float, NativeBool32> NativeButton;
    [NativeFunction("UISmallButton")]
    internal static unsafe delegate*<NativeString, NativeBool32> NativeSmallButton;
    [NativeFunction("UIInvisibleButton")]
    internal static unsafe delegate*<NativeString, float, float, NativeBool32> NativeInvisibleButton;
    [NativeFunction("UICheckbox")]
    internal static unsafe delegate*<NativeString, NativeBool32*, NativeBool32> NativeCheckbox;
    [NativeFunction("UIRadioButton")]
    internal static unsafe delegate*<NativeString, NativeBool32, NativeBool32> NativeRadioButton;
    [NativeFunction("UIProgressBar")]
    internal static unsafe delegate*<float, float, float, NativeString, void> NativeProgressBar;
    [NativeFunction("UISeparator")]
    internal static unsafe delegate*<void> NativeSeparator;
    [NativeFunction("UISameLine")]
    internal static unsafe delegate*<float, float, void> NativeSameLine;
    [NativeFunction("UISpacing")]
    internal static unsafe delegate*<void> NativeSpacing;
    [NativeFunction("UIIndent")]
    internal static unsafe delegate*<float, void> NativeIndent;
    [NativeFunction("UIUnindent")]
    internal static unsafe delegate*<float, void> NativeUnindent;
    [NativeFunction("UINewLine")]
    internal static unsafe delegate*<void> NativeNewLine;
    [NativeFunction("UIDummy")]
    internal static unsafe delegate*<float, float, void> NativeDummy;
    [NativeFunction("UIInputText")]
    internal static unsafe delegate*<NativeString, NativeString, int, NativeBool32> NativeInputText;
    [NativeFunction("UIInputTextMultiline")]
    internal static unsafe delegate*<NativeString, NativeString, int, NativeBool32> NativeInputTextMultiline;
    [NativeFunction("UIInputFloat")]
    internal static unsafe delegate*<NativeString, float*, float, float, int, NativeBool32> NativeInputFloat;
    [NativeFunction("UIInputFloat2")]
    internal static unsafe delegate*<NativeString, float*, NativeBool32> NativeInputFloat2;
    [NativeFunction("UIInputFloat3")]
    internal static unsafe delegate*<NativeString, float*, NativeBool32> NativeInputFloat3;
    [NativeFunction("UIInputFloat4")]
    internal static unsafe delegate*<NativeString, float*, NativeBool32> NativeInputFloat4;
    [NativeFunction("UIInputInt")]
    internal static unsafe delegate*<NativeString, int*, int, int, NativeBool32> NativeInputInt;
    [NativeFunction("UIDragFloat")]
    internal static unsafe delegate*<NativeString, float*, float, float, float, NativeBool32> NativeDragFloat;
    [NativeFunction("UIDragFloat3")]
    internal static unsafe delegate*<NativeString, float*, float, float, float, NativeBool32> NativeDragFloat3;
    [NativeFunction("UISliderFloat")]
    internal static unsafe delegate*<NativeString, float*, float, float, NativeBool32> NativeSliderFloat;
    [NativeFunction("UISliderInt")]
    internal static unsafe delegate*<NativeString, int*, int, int, NativeBool32> NativeSliderInt;
    [NativeFunction("UIColorEdit3")]
    internal static unsafe delegate*<NativeString, float*, NativeBool32> NativeColorEdit3;
    [NativeFunction("UIColorEdit4")]
    internal static unsafe delegate*<NativeString, float*, NativeBool32> NativeColorEdit4;
    [NativeFunction("UITreeNode")]
    internal static unsafe delegate*<NativeString, NativeBool32> NativeTreeNode;
    [NativeFunction("UITreeNodeEx")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeTreeNodeEx;
    [NativeFunction("UITreePop")]
    internal static unsafe delegate*<void> NativeTreePop;
    [NativeFunction("UICollapsingHeader")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeCollapsingHeader;
    [NativeFunction("UISelectable")]
    internal static unsafe delegate*<NativeString, NativeBool32, int, NativeBool32> NativeSelectable;
    [NativeFunction("UIBeginCombo")]
    internal static unsafe delegate*<NativeString, NativeString, int, NativeBool32> NativeBeginCombo;
    [NativeFunction("UIEndCombo")]
    internal static unsafe delegate*<void> NativeEndCombo;
    [NativeFunction("UIBeginListBox")]
    internal static unsafe delegate*<NativeString, float, float, NativeBool32> NativeBeginListBox;
    [NativeFunction("UIEndListBox")]
    internal static unsafe delegate*<void> NativeEndListBox;
    [NativeFunction("UIBeginTabBar")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginTabBar;
    [NativeFunction("UIEndTabBar")]
    internal static unsafe delegate*<void> NativeEndTabBar;
    [NativeFunction("UIBeginTabItem")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginTabItem;
    [NativeFunction("UIEndTabItem")]
    internal static unsafe delegate*<void> NativeEndTabItem;
    [NativeFunction("UIBeginMenuBar")]
    internal static unsafe delegate*<NativeBool32> NativeBeginMenuBar;
    [NativeFunction("UIEndMenuBar")]
    internal static unsafe delegate*<void> NativeEndMenuBar;
    [NativeFunction("UIBeginMainMenuBar")]
    internal static unsafe delegate*<NativeBool32> NativeBeginMainMenuBar;
    [NativeFunction("UIEndMainMenuBar")]
    internal static unsafe delegate*<void> NativeEndMainMenuBar;
    [NativeFunction("UIBeginMenu")]
    internal static unsafe delegate*<NativeString, NativeBool32, NativeBool32> NativeBeginMenu;
    [NativeFunction("UIEndMenu")]
    internal static unsafe delegate*<void> NativeEndMenu;
    [NativeFunction("UIMenuItem")]
    internal static unsafe delegate*<NativeString, NativeString, NativeBool32, NativeBool32, NativeBool32> NativeMenuItem;
    [NativeFunction("UIOpenPopup")]
    internal static unsafe delegate*<NativeString, void> NativeOpenPopup;
    [NativeFunction("UIBeginPopup")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginPopup;
    [NativeFunction("UIBeginPopupModal")]
    internal static unsafe delegate*<NativeString, int, NativeBool32> NativeBeginPopupModal;
    [NativeFunction("UIEndPopup")]
    internal static unsafe delegate*<void> NativeEndPopup;
    [NativeFunction("UICloseCurrentPopup")]
    internal static unsafe delegate*<void> NativeCloseCurrentPopup;
    [NativeFunction("UIBeginTable")]
    internal static unsafe delegate*<NativeString, int, int, NativeBool32> NativeBeginTable;
    [NativeFunction("UIEndTable")]
    internal static unsafe delegate*<void> NativeEndTable;
    [NativeFunction("UITableNextRow")]
    internal static unsafe delegate*<int, float, void> NativeTableNextRow;
    [NativeFunction("UITableNextColumn")]
    internal static unsafe delegate*<NativeBool32> NativeTableNextColumn;
    [NativeFunction("UITableSetColumnIndex")]
    internal static unsafe delegate*<int, NativeBool32> NativeTableSetColumnIndex;
    [NativeFunction("UITableSetupColumn")]
    internal static unsafe delegate*<NativeString, int, float, void> NativeTableSetupColumn;
    [NativeFunction("UITableHeadersRow")]
    internal static unsafe delegate*<void> NativeTableHeadersRow;
    [NativeFunction("UIGetContentRegionAvail")]
    internal static unsafe delegate*<float*, float*, void> NativeGetContentRegionAvail;
    [NativeFunction("UIGetWindowSize")]
    internal static unsafe delegate*<float*, float*, void> NativeGetWindowSize;
    [NativeFunction("UIGetWindowPos")]
    internal static unsafe delegate*<float*, float*, void> NativeGetWindowPos;
    [NativeFunction("UISetNextWindowSize")]
    internal static unsafe delegate*<float, float, int, void> NativeSetNextWindowSize;
    [NativeFunction("UISetNextWindowPos")]
    internal static unsafe delegate*<float, float, int, void> NativeSetNextWindowPos;
    [NativeFunction("UIIsItemHovered")]
    internal static unsafe delegate*<NativeBool32> NativeIsItemHovered;
    [NativeFunction("UIIsItemClicked")]
    internal static unsafe delegate*<int, NativeBool32> NativeIsItemClicked;
    [NativeFunction("UIIsItemActive")]
    internal static unsafe delegate*<NativeBool32> NativeIsItemActive;
    [NativeFunction("UIIsWindowFocused")]
    internal static unsafe delegate*<int, NativeBool32> NativeIsWindowFocused;
    [NativeFunction("UIIsWindowHovered")]
    internal static unsafe delegate*<int, NativeBool32> NativeIsWindowHovered;
    [NativeFunction("UIPushStyleColor")]
    internal static unsafe delegate*<int, float, float, float, float, void> NativePushStyleColor;
    [NativeFunction("UIPopStyleColor")]
    internal static unsafe delegate*<int, void> NativePopStyleColor;
    [NativeFunction("UIPushStyleVarFloat")]
    internal static unsafe delegate*<int, float, void> NativePushStyleVarFloat;
    [NativeFunction("UIPushStyleVarVec2")]
    internal static unsafe delegate*<int, float, float, void> NativePushStyleVarVec2;
    [NativeFunction("UIPopStyleVar")]
    internal static unsafe delegate*<int, void> NativePopStyleVar;
    [NativeFunction("UIPushItemWidth")]
    internal static unsafe delegate*<float, void> NativePushItemWidth;
    [NativeFunction("UIPopItemWidth")]
    internal static unsafe delegate*<void> NativePopItemWidth;
    [NativeFunction("UIPushIDStr")]
    internal static unsafe delegate*<NativeString, void> NativePushIDStr;
    [NativeFunction("UIPushIDInt")]
    internal static unsafe delegate*<int, void> NativePushIDInt;
    [NativeFunction("UIPopID")]
    internal static unsafe delegate*<void> NativePopID;
    [NativeFunction("UISetTooltip")]
    internal static unsafe delegate*<NativeString, void> NativeSetTooltip;
    [NativeFunction("UIBeginTooltip")]
    internal static unsafe delegate*<NativeBool32> NativeBeginTooltip;
    [NativeFunction("UIEndTooltip")]
    internal static unsafe delegate*<void> NativeEndTooltip;
    [NativeFunction("UIBeginDragDropSource")]
    internal static unsafe delegate*<NativeBool32> NativeBeginDragDropSource;
    [NativeFunction("UISetDragDropPayload")]
    internal static unsafe delegate*<IntPtr, void> NativeSetDragDropPayload;
    [NativeFunction("UIEndDragDropSource")]
    internal static unsafe delegate*<void> NativeEndDragDropSource;
    [NativeFunction("UIBeginDragDropTarget")]
    internal static unsafe delegate*<NativeBool32> NativeBeginDragDropTarget;
    [NativeFunction("UIEndDragDropTarget")]
    internal static unsafe delegate*<void> NativeEndDragDropTarget;

    public static bool BeginWindow(string title, int flags)
    {
      NativeString title_native = title;
      bool open = false;
      unsafe
      {
        NativeBool32 result = NativeBeginWindow(title_native, flags);
        open = result;
      }
      return open;
    }

    public static void EndWindow()
    {
      unsafe
      {
        NativeEndWindow();
      }
    }

    public static bool BeginChild(string name, int flags)
    {
      NativeString name_native = name;
      bool open = false;
      unsafe
      {
        NativeBool32 result = NativeBeginChild(name_native, flags);
        open = result;
      }
      return open;
    }

    public static void EndChild()
    {
      unsafe
      {
        NativeEndChild();
      }
    }

    public static void Text(string text)
    {
      unsafe { NativeText(text); }
    }

    public static void TextColored(float r, float g, float b, float a, string text)
    {
      unsafe { 
        NativeString str = text;
        NativeTextColored(r, g, b, a, str); 
      }
    }

    public static void TextColored(Vec4 color, string text)
    {
      unsafe { 
        NativeString str = text;
        NativeTextColored(color.X, color.Y, color.Z, color.W, str); 
      }
    }

    public static void TextWrapped(string text)
    {
      unsafe { 
        NativeString str = text;
        NativeTextWrapped(str); 
      }
    }

    public static void LabelText(string label, string text)
    {
      unsafe { 
        NativeString label_str = label;
        NativeString text_str = text;
        NativeLabelText(label_str, text_str); 
      }
    }

    public static bool Button(string label, float width = 0, float height = 0)
    {
      unsafe { return NativeButton(label, width, height); }
    }

    public static bool SmallButton(string label)
    {
      unsafe { return NativeSmallButton(label); }
    }

    public static bool InvisibleButton(string str_id, float width, float height)
    {
      unsafe { return NativeInvisibleButton(str_id, width, height); }
    }

    public static bool Checkbox(string label, ref bool value)
    {
      unsafe
      {
        NativeBool32 nb = value;
        bool changed = NativeCheckbox(label, &nb);
        value = nb;
        return changed;
      }
    }

    public static bool RadioButton(string label, bool active)
    {
      unsafe { return NativeRadioButton(label, active); }
    }

    public static void ProgressBar(float fraction, float width = -1, float height = 0, string overlay = "")
    {
      unsafe { NativeProgressBar(fraction, width, height, overlay); }
    }

    public static void Separator() { unsafe { NativeSeparator(); } }
    public static void SameLine(float offset = 0, float spacing = -1) { unsafe { NativeSameLine(offset, spacing); } }
    public static void Spacing() { unsafe { NativeSpacing(); } }
    public static void Indent(float width = 0) { unsafe { NativeIndent(width); } }
    public static void Unindent(float width = 0) { unsafe { NativeUnindent(width); } }
    public static void NewLine() { unsafe { NativeNewLine(); } }
    public static void Dummy(float width, float height) { unsafe { NativeDummy(width, height); } }

    public static bool InputText(string label, string text, int max_length)
    {
      NativeString label_native = label;
      NativeString text_native = text;
      unsafe
      {
        NativeBool32 result = NativeInputTextMultiline(label_native, text_native, max_length);
        return result;
      }
    }

    public static bool InputTextMultiline(string label, ref string text, int max_length)
    {
      NativeString label_native = label;
      NativeString text_native = text;
      unsafe
      {
        NativeBool32 result = NativeInputTextMultiline(label_native, text_native, max_length);
        if (result) 
        {
          text = text_native;
        }
        return result;
      }
    }

    public static bool InputFloat(string label, ref float value, float step = 0, float step_fast = 0, int decimal_precision = 3)
    {
      unsafe
      {
        fixed (float* ptr = &value)
          return NativeInputFloat(label, ptr, step, step_fast, decimal_precision);
      }
    }

    public static bool InputFloat2(string label, float[] values)
    {
      unsafe
      {
        fixed (float* ptr = values)
          return NativeInputFloat2(label, ptr);
      }
    }

    public static bool InputFloat3(string label, float[] values)
    {
      unsafe
      {
        fixed (float* ptr = values)
          return NativeInputFloat3(label, ptr);
      }
    }

    public static bool InputFloat3(string label, ref Vec3 value)
    {
      float[] arr = new float[] { value.X, value.Y, value.Z };
      unsafe
      {
        fixed (float* ptr = arr)
        {
          bool changed = NativeInputFloat3(label, ptr);
          if (changed) value = new Vec3(arr[0], arr[1], arr[2]);
          return changed;
        }
      }
    }

    public static bool InputFloat4(string label, float[] values)
    {
      unsafe
      {
        fixed (float* ptr = values)
          return NativeInputFloat4(label, ptr);
      }
    }

    public static bool InputInt(string label, ref int value, int step = 1, int step_fast = 100)
    {
      unsafe
      {
        fixed (int* ptr = &value)
          return NativeInputInt(label, ptr, step, step_fast);
      }
    }

    public static bool DragFloat(string label, ref float value, float speed = 1.0f, float min = 0, float max = 0)
    {
      unsafe
      {
        fixed (float* ptr = &value)
          return NativeDragFloat(label, ptr, speed, min, max);
      }
    }

    public static bool DragFloat3(string label, ref Vec3 value, float speed = 1.0f, float min = 0, float max = 0)
    {
      float[] arr = new float[] { value.X, value.Y, value.Z };
      unsafe
      {
        fixed (float* ptr = arr)
        {
          bool changed = NativeDragFloat3(label, ptr, speed, min, max);
          if (changed) value = new Vec3(arr[0], arr[1], arr[2]);
          return changed;
        }
      }
    }

    public static bool SliderFloat(string label, ref float value, float min, float max)
    {
      unsafe
      {
        fixed (float* ptr = &value)
          return NativeSliderFloat(label, ptr, min, max);
      }
    }

    public static bool SliderInt(string label, ref int value, int min, int max)
    {
      unsafe
      {
        fixed (int* ptr = &value)
          return NativeSliderInt(label, ptr, min, max);
      }
    }

    public static bool ColorEdit3(string label, ref Vec3 color)
    {
      float[] arr = new float[] { color.X, color.Y, color.Z };
      unsafe
      {
        fixed (float* ptr = arr)
        {
          bool changed = NativeColorEdit3(label, ptr);
          if (changed) color = new Vec3(arr[0], arr[1], arr[2]);
          return changed;
        }
      }
    }

    public static bool ColorEdit4(string label, ref Vec4 color)
    {
      float[] arr = new float[] { color.X, color.Y, color.Z, color.W };
      unsafe
      {
        fixed (float* ptr = arr)
        {
          bool changed = NativeColorEdit4(label, ptr);
          if (changed) color = new Vec4(arr[0], arr[1], arr[2], arr[3]);
          return changed;
        }
      }
    }

    public static bool TreeNode(string label) { unsafe { return NativeTreeNode(label); } }
    public static bool TreeNodeEx(string label, int flags = 0) { unsafe { return NativeTreeNodeEx(label, flags); } }
    public static void TreePop() { unsafe { NativeTreePop(); } }
    public static bool CollapsingHeader(string label, int flags = 0) { unsafe { return NativeCollapsingHeader(label, flags); } }

    public static bool Selectable(string label, bool selected = false, int flags = 0) { unsafe { return NativeSelectable(label, selected, flags); } }
    public static bool BeginCombo(string label, string preview_value, int flags = 0) { unsafe { return NativeBeginCombo(label, preview_value, flags); } }
    public static void EndCombo() { unsafe { NativeEndCombo(); } }
    public static bool BeginListBox(string label, float width = 0, float height = 0) { unsafe { return NativeBeginListBox(label, width, height); } }
    public static void EndListBox() { unsafe { NativeEndListBox(); } }

    public static bool BeginTabBar(string str_id, int flags = 0) { unsafe { return NativeBeginTabBar(str_id, flags); } }
    public static void EndTabBar() { unsafe { NativeEndTabBar(); } }
    public static bool BeginTabItem(string label, int flags = 0) { unsafe { return NativeBeginTabItem(label, flags); } }
    public static void EndTabItem() { unsafe { NativeEndTabItem(); } }

    public static bool BeginMenuBar() { unsafe { return NativeBeginMenuBar(); } }
    public static void EndMenuBar() { unsafe { NativeEndMenuBar(); } }
    public static bool BeginMainMenuBar() { unsafe { return NativeBeginMainMenuBar(); } }
    public static void EndMainMenuBar() { unsafe { NativeEndMainMenuBar(); } }
    public static bool BeginMenu(string label, bool enabled = true) { unsafe { return NativeBeginMenu(label, enabled); } }
    public static void EndMenu() { unsafe { NativeEndMenu(); } }
    public static bool MenuItem(string label, string shortcut = "", bool selected = false, bool enabled = true)
    {
      unsafe { return NativeMenuItem(label, shortcut, selected, enabled); }
    }

    public static void OpenPopup(string str_id) { unsafe { NativeOpenPopup(str_id); } }
    public static bool BeginPopup(string str_id, int flags = 0) { unsafe { return NativeBeginPopup(str_id, flags); } }
    public static bool BeginPopupModal(string name, int flags = 0) { unsafe { return NativeBeginPopupModal(name, flags); } }
    public static void EndPopup() { unsafe { NativeEndPopup(); } }
    public static void CloseCurrentPopup() { unsafe { NativeCloseCurrentPopup(); } }

    public static bool BeginTable(string str_id, int columns, int flags = 0) { unsafe { return NativeBeginTable(str_id, columns, flags); } }
    public static void EndTable() { unsafe { NativeEndTable(); } }
    public static void TableNextRow(int flags = 0, float min_row_height = 0) { unsafe { NativeTableNextRow(flags, min_row_height); } }
    public static bool TableNextColumn() { unsafe { return NativeTableNextColumn(); } }
    public static bool TableSetColumnIndex(int column_n) { unsafe { return NativeTableSetColumnIndex(column_n); } }
    public static void TableSetupColumn(string label, int flags = 0, float init_width = 0) { unsafe { NativeTableSetupColumn(label, flags, init_width); } }
    public static void TableHeadersRow() { unsafe { NativeTableHeadersRow(); } }

    public static Vec2 GetContentRegionAvail()
    {
      float x = 0, y = 0;
      unsafe { NativeGetContentRegionAvail(&x, &y); }
      return new Vec2(x, y);
    }

    public static Vec2 GetWindowSize()
    {
      float w = 0, h = 0;
      unsafe { NativeGetWindowSize(&w, &h); }
      return new Vec2(w, h);
    }

    public static Vec2 GetWindowPos()
    {
      float x = 0, y = 0;
      unsafe { NativeGetWindowPos(&x, &y); }
      return new Vec2(x, y);
    }

    public static void SetNextWindowSize(float width, float height, int cond = 0) { unsafe { NativeSetNextWindowSize(width, height, cond); } }
    public static void SetNextWindowSize(Vec2 size, int cond = 0) { unsafe { NativeSetNextWindowSize(size.X, size.Y, cond); } }
    public static void SetNextWindowPos(float x, float y, int cond = 0) { unsafe { NativeSetNextWindowPos(x, y, cond); } }
    public static void SetNextWindowPos(Vec2 pos, int cond = 0) { unsafe { NativeSetNextWindowPos(pos.X, pos.Y, cond); } }
    public static bool IsItemHovered() { unsafe { return NativeIsItemHovered(); } }
    public static bool IsItemClicked(int mouse_button = 0) { unsafe { return NativeIsItemClicked(mouse_button); } }
    public static bool IsItemActive() { unsafe { return NativeIsItemActive(); } }
    public static bool IsWindowFocused(int flags = 0) { unsafe { return NativeIsWindowFocused(flags); } }
    public static bool IsWindowHovered(int flags = 0) { unsafe { return NativeIsWindowHovered(flags); } }

    public static void PushStyleColor(int idx, Vec4 color) { unsafe { NativePushStyleColor(idx, color.X, color.Y, color.Z, color.W); } }
    public static void PushStyleColor(int idx, float r, float g, float b, float a) { unsafe { NativePushStyleColor(idx, r, g, b, a); } }
    public static void PopStyleColor(int count = 1) { unsafe { NativePopStyleColor(count); } }
    public static void PushStyleVar(int idx, float val) { unsafe { NativePushStyleVarFloat(idx, val); } }
    public static void PushStyleVar(int idx, Vec2 val) { unsafe { NativePushStyleVarVec2(idx, val.X, val.Y); } }
    public static void PopStyleVar(int count = 1) { unsafe { NativePopStyleVar(count); } }
    public static void PushItemWidth(float width) { unsafe { NativePushItemWidth(width); } }
    public static void PopItemWidth() { unsafe { NativePopItemWidth(); } }

    public static void PushID(string str_id) { unsafe { NativePushIDStr(str_id); } }
    public static void PushID(int int_id) { unsafe { NativePushIDInt(int_id); } }
    public static void PopID() { unsafe { NativePopID(); } }

    public static void SetTooltip(string text) { unsafe { NativeSetTooltip(text); } }
    public static bool BeginTooltip() { unsafe { return NativeBeginTooltip(); } }
    public static void EndTooltip() { unsafe { NativeEndTooltip(); } }

    public static bool BeginDragDropSource() { unsafe { return NativeBeginDragDropSource(); } }
    public static void SetDragDropPayload(IntPtr payload) { unsafe { NativeSetDragDropPayload(payload); } }
    public static void EndDragDropSource() { unsafe { NativeEndDragDropSource(); } }
    public static bool BeginDragDropTarget() { unsafe { return NativeBeginDragDropTarget(); } }
    public static void EndDragDropTarget() { unsafe { NativeEndDragDropTarget(); } }

    public static void HelpMarker(string description)
    {
      if (IsItemHovered())
      {
        BeginTooltip();
        Text(description);
        EndTooltip();
      }
    }
  }
}