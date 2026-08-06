using Other;
using Other.Core;

public class TestBehavior : SceneBehavior
{
  public float rotation_speed = 90.0f; // degrees per second

  protected override void OnUpdate()
  {
    Transform.EulerAngles += Vec3.Up * rotation_speed * Time.DeltaTime;
  }
}