namespace Other
{
  public static class RenderingComponentAccess
  {
    [DllImport("OtherNative")]
    private static extern Vec3 GetPosition(ulong object_id);

    [DllImport("OtherNative")]
    private static extern void SetPosition(ulong object_id, Vec3 position);

    [DllImport("OtherNative")]
    private static extern Quaternion GetRotation(ulong object_id);

    [DllImport("OtherNative")]
    private static extern void SetRotation(ulong object_id, Quaternion rotation);

    [DllImport("OtherNative")]
    private static extern Vec3 GetScale(ulong object_id);

    [DllImport("OtherNative")]
    private static extern void SetScale(ulong object_id, Vec3 scale);

    [DllImport("OtherNative")]
    private static extern Mat4 GetWorldMatrix(ulong object_id);
  }
}