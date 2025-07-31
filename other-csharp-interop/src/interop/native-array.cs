using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;

#nullable enable
namespace OtherCsBindings
{
  public sealed class NativeArrayEnumerator<T> : IEnumerator<T>
  {
    private readonly T[] array;
    private Int32 index;

    public NativeArrayEnumerator(T[] array)
    {
      this.array = array;
      index = -1;
    }

    public void Dispose()
    {
      index = -1;
      GC.SuppressFinalize(this);
    }

    public bool MoveNext() => ++index < array.Length;
    public void Reset() => index = -1;

    void IEnumerator.Reset() => index = -1;
    void IDisposable.Dispose()
    {
      index = -1;
      GC.SuppressFinalize(this);
    }
    object IEnumerator.Current => Current!;
    public T Current => array[index];
  }

  [StructLayout(LayoutKind.Sequential, Pack = 1)]
  public sealed class NativeArray<T> : IEnumerable<T>
  {
    private readonly IntPtr array;
    private readonly Int32 length;
    private NativeBool32 disposed;

    public Int32 Length => length;

    public NativeArray(IntPtr len)
    {
      this.array = Marshal.AllocHGlobal(len * Marshal.SizeOf<T>());
      this.length = Marshal.ReadInt32(len);
    }

    public NativeArray([DisallowNull] T?[] arr)
    {
      this.array = Marshal.AllocHGlobal(arr.Length * Marshal.SizeOf<T>());
      this.length = arr.Length;

      for (int i = 0; i < arr.Length; i++)
      {
        var elem = arr[i];
        if (elem == null)
        {
          continue;
        }

        Marshal.StructureToPtr(arr[i]!, IntPtr.Add(this.array, i * Marshal.SizeOf<T>()), false);
      }
    }

    internal NativeArray(IntPtr array, int length)
    {
      this.array = array;
      this.length = length;
    }

    public T[] ToArray()
    {
      Span<T> data = Span<T>.Empty;
      if (array != IntPtr.Zero && length > 0)
      {
        unsafe
        {
          data = new Span<T>(array.ToPointer(), length);
        }
      }
      return data.ToArray();
    }

    public Span<T> ToSpan()
    {
      unsafe
      {
        return new Span<T>(array.ToPointer(), length);
      }
    }

    public ReadOnlySpan<T> ToReadOnlySpan() => ToSpan();

    public void Dispose()
    {
      if (!disposed)
      {
        if (array != IntPtr.Zero)
        {
          Marshal.FreeHGlobal(array);
        }

        disposed = true;
      }

      GC.SuppressFinalize(this);
    }

    public IEnumerator<T> GetEnumerator() => new NativeArrayEnumerator<T>(this);
    IEnumerator IEnumerable.GetEnumerator() => new NativeArrayEnumerator<T>(this);

    public T? this[Int32 index]
    {
      get => Marshal.PtrToStructure<T>(IntPtr.Add(array, index * Marshal.SizeOf<T>()));
      set => Marshal.StructureToPtr<T>(value!, IntPtr.Add(array, index * Marshal.SizeOf<T>()), false);
    }

    public static implicit operator T[](NativeArray<T> array) => array.ToArray();
  }
}
#nullable disable