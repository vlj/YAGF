using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Interop;

namespace mesh_vulkan_managed
{
    class HostedGraphicSurface : HwndHost
    {
        internal const int
            WS_CHILD = 0x40000000,
             WS_VISIBLE = 0x10000000,
            LBS_NOTIFY = 0x00000001,
            HOST_ID = 0x00000002,
            LISTBOX_ID = 0x00000001,
            WS_VSCROLL = 0x00200000,
            WS_BORDER = 0x00800000;

        [DllImport("user32.dll", EntryPoint = "CreateWindowEx", CharSet = CharSet.Unicode)]
        internal static extern IntPtr CreateWindowEx(int dwExStyle,
                                              string lpszClassName,
                                              string lpszWindowName,
                                              int style,
                                              int x, int y,
                                              int width, int height,
                                              IntPtr hwndParent,
                                              IntPtr hMenu,
                                              IntPtr hInst,
                                              [MarshalAs(UnmanagedType.AsAny)] object pvParam);

        [DllImport("mesh.vulkan.dll")]
        internal static extern IntPtr create_vulkan_mesh(IntPtr Hwnd, IntPtr hInstance);
        [DllImport("mesh.vulkan.dll")]
        internal static extern void draw_vulkan_mesh(IntPtr mesh_ptr);
        [DllImport("mesh.vulkan.dll")]
        internal static extern void destroy_vulkan_mesh(IntPtr mesh_ptr);
        static IntPtr mesh;

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            // create the window and use the result as the handle
            var hWnd = CreateWindowEx(0,
                "static",    // name of the window class
                "",   // title of the window
                WS_CHILD,    // window style
                0,    // x-position of the window
                0,    // y-position of the window
                1024,    // width of the window
                1024,    // height of the window
                hwndParent.Handle,
                IntPtr.Zero,    // we aren't using menus, NULL
                IntPtr.Zero,    // application handle
                IntPtr.Zero);    // used with multiple windows, NULL

            mesh = create_vulkan_mesh(hWnd, Marshal.GetHINSTANCE(typeof(App).Module));
            draw_vulkan_mesh(mesh);
            return new HandleRef(this, hWnd);
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            destroy_vulkan_mesh(mesh);
        }

        protected override IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            draw_vulkan_mesh(mesh);
            return base.WndProc(hwnd, msg, wParam, lParam, ref handled);
        }
    }
}
