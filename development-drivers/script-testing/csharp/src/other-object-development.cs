using System;
using Other;
using Other.Core;

class TestAttrAttribute : Attribute
{
  public string Name { get; set; }
  public int Value = 0;

  public TestAttrAttribute(string name, int value)
  {
    Name = name;
    Value = value;
  }
}

[TestAttr("TestObject", 42)]
class TestObject
{
  [SerializableField]
  int field_value = 10;
  public int PropertyValue { get; set; } = 20;

  string field_string = "Hello World";
  string PropertyString { get; set; } = "Hello Property";

  public TestObject()
  {
  }

  ~TestObject()
  {
  }

  public void DisplayInfo()
  {
    Debug.Log($"Filesystem.GetProgramFilesFolder(): {Filesystem.GetProgramFilesFolder("MyApp")}");
    Debug.Log($"Filesystem.GetAppDataFolder(): {Filesystem.GetAppDataFolder("MyApp")}");
    // Debug.Log($"Field Value: {field_value}");
    // Debug.Log($"Property Value: {PropertyValue}");
    // Debug.Log($"Field String: {field_string}");
    // Debug.Log($"Property String: {PropertyString}");
  }

  public int GetValue(int value)
  {
    Debug.Log($"Received value: {value}");
    return value * 2;
  }
}