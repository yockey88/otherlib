using System;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Linq;
using System.Net.Security;

#nullable enable
namespace OtherCsBindings
{
  public class NativeFunctionManager
  {
    [UnmanagedCallersOnly]
    private static void RegisterInternalCall(NativeString name_str, IntPtr target)
    {
      try
      {
        var name = name_str.ToString();

        var name_start = name!.IndexOf('+');
        var name_end = name!.IndexOf(",", name_start, StringComparison.CurrentCulture);
        var field_name = name!.Substring(name_start + 1, name_end - name_start - 1);
        var containing_type_name = name!.Remove(name_start, name_end - name_start);

        var type = InteropInterface.FindType(containing_type_name);

        if (type == null)
        {
          return;
        }

        var binding_flags = BindingFlags.Static | BindingFlags.NonPublic;
        var field = type.GetFields(binding_flags).FirstOrDefault(field => field.Name == field_name);
        if (field == null)
        {
          return;
        }

        var field_type = field.FieldType;
        if (!field.FieldType.IsFunctionPointer)
        {
          return;
        }

        field.SetValue(null, target);
      }
      catch (Exception ex)
      {
        Host.HandleException(ex);
      }
    }
  }
}
#nullable disable