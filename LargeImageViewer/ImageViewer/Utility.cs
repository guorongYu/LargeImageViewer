using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace ImageViewer
{
    public static class Utility
    {
        public static BitmapSource MakeBitmapSourceFromLayer(int layer)
        {
            var incoming = new byte[ImageGrabber.GetLayerBufferPitch(layer)];
            unsafe
            {
                fixed (byte* inBuf = incoming)
                {
                    byte* outBuf = (byte*)ImageGrabber.GetLayerData((IntPtr)inBuf, layer);
                }
            }

            int width = ImageGrabber.GetLayerWidth(layer);
            int height = ImageGrabber.GetLayerHeight(layer);
            int stride = width;
            BitmapSource bitmapSource =
                BitmapSource.Create(width,
                                    height,
                                    300,
                                    300,
                                    PixelFormats.Gray8,
                                    BitmapPalettes.Gray256,
                                    incoming, stride);
            return bitmapSource;
        }
        public static BitmapSource MakeBitmapSourceFromRegionOfLayer(int layer, Rect region)
        {
            int left = (int)region.left;
            int top = (int)region.top;
            int right = (int)region.right;
            int bottom = (int)region.bottom;
            int width = right - left;
            int height = bottom - top;
            int size = width * height;
            int stride = width;
            var incoming = new byte[width * height];
            unsafe
            {
                fixed (byte* inBuf = incoming)
                {
                    byte* outBuf = 
                        (byte*)ImageGrabber.GetLayerDataOfRegion((IntPtr)inBuf, 
                                                                 layer,
                                                                 left,
                                                                 top,
                                                                 right,
                                                                 bottom);
                }
            }
            BitmapSource bitmapSource =
                BitmapSource.Create(width,
                                    height,
                                    300,
                                    300,
                                    PixelFormats.Gray8,
                                    BitmapPalettes.Gray256,
                                    incoming, stride);
            return bitmapSource;
        }
    }
}
