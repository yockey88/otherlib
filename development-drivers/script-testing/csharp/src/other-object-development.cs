using System;

namespace Other
{
  class TestAttrAttribute : Attribute
  {
    public string Name { get; set; }
    public int Value { get; set; }

    public TestAttrAttribute(string name, int value)
    {
      Console.WriteLine($"TestAttrAttribute created with Name: {name}, Value: {value}");
      Name = name;
      Value = value;
    }
  }

  [TestAttr("TestObject", 42)]
  class TestObject
  {
    int field_value = 10;
    public int PropertyValue { get; set; } = 20;

    public TestObject()
    {
      Console.WriteLine("TestObject instantiated");
    }

    ~TestObject()
    {
      Console.WriteLine("TestObject finalized");
    }

    public void DisplayInfo()
    {
      Console.WriteLine($"Hello There! I am an instance of {GetType().Name}.");
    }

    public int GetValue(int value)
    {
      Console.WriteLine($"Received value: {value}");
      return value * 2;
    }
  }
} 