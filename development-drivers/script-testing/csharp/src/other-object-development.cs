using System;

namespace Other
{
  class Object
  {
    public string Name { get; set; }
    public int Id { get; set; }
    public IntPtr NativeHandle { get; set; }

    public Object(string name, int id, IntPtr nativeHandle)
    {
      Name = name;
      Id = id;
      NativeHandle = nativeHandle;
    }

    public void DisplayInfo()
    {
      Console.WriteLine($"Object Name: {Name}, ID: {Id}, Native Handle: {NativeHandle}");
    }
  }
} 