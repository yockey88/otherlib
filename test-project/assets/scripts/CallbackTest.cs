using Other;

namespace Testing
{
  public class CallbackTest
  {
    [CallbackBinding("Test.Callback")]
    public static void TestCallback()
    {
      Debug.Log("TestCallback invoked from native code!");
    }
  }
}