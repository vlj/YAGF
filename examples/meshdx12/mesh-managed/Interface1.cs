using System;
using System.Runtime.InteropServices;

namespace mesh_managed
{
    [ComImport, Guid("A15DDC0D-53EF-4776-8DA2-E87399C6654D"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface Interface1
    {
        [return: MarshalAs(UnmanagedType.LPWStr)]
        string HelloWorld([MarshalAs(UnmanagedType.LPWStr)] string name);
    }
}
