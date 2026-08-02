using System;
using Other;
using Other.Core;

namespace Testing
{
    public class MainGraph : SceneBehavior
    {
        protected override void OnEnable()
        {
            Console.WriteLine("Hello from C#!");
        }

        protected override void OnDisable()
        {
            Console.WriteLine("Goodbye from C#!");
        }
    }
}