using OtherCsBindings;

namespace Other
{
  public static class Fnv
  {
    public static ulong Hash(string str)
    {
      unsafe
      {
        var ns = new NativeString(str);
        ulong result = OtherABI.NativeFnvHash(ns);
        ns.Dispose();
        return result;
      }
    }
  }
}
