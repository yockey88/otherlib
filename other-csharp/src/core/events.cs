using System;
using OtherCsBindings;

namespace Other.Core
{
  public static class Events
  {
    [NativeFunction("EventRegister")]
    internal static unsafe delegate*<NativeString, void> NativeRegister;
    [NativeFunction("EventTrigger")]
    internal static unsafe delegate*<NativeString, void> NativeTrigger;
    [NativeFunction("EventTriggerWithString")]
    internal static unsafe delegate*<NativeString, NativeString, void> NativeTriggerWithString;

    public static void Register(string event_name)
    {
      unsafe { NativeRegister(event_name); }
    }

    public static void Trigger(string event_name)
    {
      unsafe { NativeTrigger(event_name); }
    }

    public static void Trigger<T>(string event_name, T data)
    {
      /// have to do different things depending on the type of data
      if (data is string str_data)
      {
        Trigger(event_name, str_data);
      }
      else
      {
        throw new NotSupportedException("Only string data is supported.");
      }
    }

    public static void Trigger(string event_name, string data)
    {
      unsafe { NativeTriggerWithString(event_name, data); }
    }
  }
}