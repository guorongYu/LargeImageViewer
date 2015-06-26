using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ImageViewer
{
    public class Rect
    {
        public double left;
        public double top;
        public double right;
        public double bottom;
    }

    public class Bound
    {
        public Point topLeft;
        public Point topRight;
        public Point botLeft;
        public Point botRight;
        public Bound()
        {
            topLeft = new Point();
            topRight = new Point();
            botLeft = new Point();
            botRight = new Point();
        }
    }

    public class Point
    {
        public double x;
        public double y;
        public void Set(double a, double b)
        {
            x = a;
            y = b;
        }
    }

    public class ImageInfo
    {
        public double width;
        public double height;
    }

    public enum SideBoxID
    {
        Undefined = 0,
        LeftTop,
        RightTop,
        LeftBottom,
        RightBottom
    }

    public struct Constants
    {
        public const double DRAG_BOX_MINIMUM_WIDTH = 50;
        public const double DRAG_BOX_MAXIMUM_WIDTH = 600;
        public const double DRAG_BOX_MINIMUM_HEIGHT = 50;
        public const double DRAG_BOX_MAXIMUM_HEIGHT = 300;
    }

    public static class Global
    {
        public static ImageGrabber.CallbackDelegate callback_delegate = null;
        public static Point beginCursorPos = new Point();
        //public static Point beginDragBoxPos;
        public static bool isSelectAreaResizing = false;
        public static bool isSelectAreaMove = false;
        public static SideBoxID sideboxID = SideBoxID.Undefined;
        public static ImageInfo currentImage = new ImageInfo();
        public static Bound bound = new Bound();
        public static double mapped_height;
    }
}
