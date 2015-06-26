using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace ImageViewer
{
    public static class ImageGrabber
    {
        public delegate void CallbackDelegate(int arg);
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int OnLoad();
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr GetLayerData(IntPtr dst, Int32 index);
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr GetLayerDataOfRegion(IntPtr dst, Int32 index, Int32 left, Int32 top, Int32 right, Int32 bottom);
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetLayerBufferPitch(Int32 index);
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetLayerHeight(Int32 index);
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetLayerWidth(Int32 index);
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool OpenConsole();
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool CloseConsole();
        [DllImport("ImageGrabber.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void AddUpdateCallback(CallbackDelegate cbFunction);
    }
}
