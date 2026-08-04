using Other;
using Other.Core;

public class TestBehavior : SceneBehavior
{
  float rotation = 0.0f;
  float rotation_speed = 90.0f; // degrees per second

  protected override void OnAwake()
  {
    Debug.Warn("TestBehavior: OnAwake called.");
  }

  protected override void OnRemove()
  {
    Debug.Warn("TestBehavior: OnRemove called.");
  }

  protected override void OnEnable()
  {
    Debug.Warn("TestBehavior: OnEnable called.");
  }

  protected override void OnDisable()
  {
    Debug.Warn("TestBehavior: OnDisable called.");
  }

  protected override void OnUpdate()
  {
    // Transform.EulerAngles += Vec3.Up * rotation_speed * Time.DeltaTime;
  }
}