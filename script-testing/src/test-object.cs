using System;
using Other;

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
  public int field_value = 10;
  public int PropertyValue { get; set; } = 20;

  public string field_string = "Hello World";
  public string PropertyString { get; set; } = "Hello Property";

  public TestObject()
  {
  }

  ~TestObject()
  {
  }

  public int Add(int a, int b)
  {
    Debug.Log($"{a + b}");
    return a + b;
  }

  public void DisplayInfo()
  {
    Console.WriteLine("TestObject Information:");
    Console.WriteLine($"Field Value: {field_value}");
    Console.WriteLine($"Property Value: {PropertyValue}");
    Console.WriteLine($"Field String: {field_string}");
    Console.WriteLine($"Property String: {PropertyString}");
  }

  public int GetValue(int value)
  {
    Debug.Log($"Received value: {value}");
    return value * 2;
  }

  [CallbackBinding("Math.Square")]
  public static int StaticMethod(int x)
  {
    Console.WriteLine($"StaticMethod called: {x} * {x} = {x * x}.");
    return x * x;
  }
}

/// mirrors the runtime SceneObject/behavior wiring for the assembly-refresh tests: a parent
///   whose behavior list is driven through the native interop, plus a minimal behavior
class TestParentObject : Other.Core.OtherObject
{
  public TestParentObject() : base(IntPtr.Zero, ObjectType.SceneObject)
  {
  }

  public int CountBehaviors() => BehaviorCount;

  public override void OnStart() { }
  public override void OnStop() { }
  public override void OnUpdate() { }
  public override void OnLateUpdate() { }
  public override void OnFixedUpdate() { }
}

class TestBehavior : Other.Core.OtherBehavior
{
  protected override void Awake() { }
  protected override void Remove() { }
  protected override void Enable() { }
  protected override void Disable() { }
  protected override void Update() { }
  protected override void LateUpdate() { }
  protected override void FixedUpdate() { }
}