using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.IO;
using System.Windows.Threading;

namespace ImageViewer
{    
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        public void OnUpdateCallback(int arg)
        {
            Application.Current.Dispatcher.BeginInvoke(
                DispatcherPriority.Background, new Action(
            delegate()
            {
                UpdateDisplayContent();
            }));
        }

        private void UpdateDisplayContent()
        {
            int display_layer = 3;
            ImageControl.Source = Utility.MakeBitmapSourceFromLayer(display_layer);
        }

        public MainWindow()
        {
            InitializeComponent();
            ImageGrabber.OpenConsole();
            ImageGrabber.OnLoad();
            Global.callback_delegate = new ImageGrabber.CallbackDelegate(OnUpdateCallback);
            ImageGrabber.AddUpdateCallback(Global.callback_delegate);
            UpdateDisplayContent();
            int width = ImageGrabber.GetLayerWidth(0);
            int height = ImageGrabber.GetLayerHeight(0);
            Global.currentImage.width = width;
            Global.currentImage.height = height;
            Global.mapped_height = (height * Content.Width) / width;
            Global.bound.topLeft.Set(0, (Content.Height - Global.mapped_height) * 0.5);
            Global.bound.topRight.Set(Content.Width, (Content.Height - Global.mapped_height) * 0.5);
            Global.bound.botLeft.Set(0, width + (Content.Height - Global.mapped_height) * 0.5);
            Global.bound.botRight.Set(Content.Width, width + (Content.Height - Global.mapped_height) * 0.5);
        }

        private void SelectAreaMouseMove(object sender, MouseEventArgs e)
        {
            if(Global.isSelectAreaMove == true)
            {
                var current_pos = e.GetPosition(ImageControl);
                Point diff = new Point();
                diff.x = Global.beginCursorPos.x - current_pos.X;
                diff.y = Global.beginCursorPos.y - current_pos.Y;
                Global.beginCursorPos.x = current_pos.X;
                Global.beginCursorPos.y = current_pos.Y;
                //double h = (Global.currentImage.height * Content.Width) / Global.currentImage.width;
                DragBox.Margin = new Thickness(Math.Max(Math.Min(DragBox.Margin.Left - diff.x, Global.bound.topRight.x - DragBox.Width), 0),
                                               Math.Max(Math.Min(DragBox.Margin.Top - diff.y, (Global.mapped_height + Global.bound.topRight.y) - DragBox.Height), 0 + Global.bound.topRight.y),
                                               Math.Max(Math.Min(DragBox.Margin.Right + diff.x, Global.bound.topRight.x - DragBox.Width), 0),
                                               Math.Max(Math.Min(DragBox.Margin.Bottom + diff.y, (Global.mapped_height + Global.bound.topRight.y) - DragBox.Height), 0 + Global.bound.topRight.y)
                                               );
            }
        }

        private void SelectArea_MouseDown(object sender, MouseEventArgs e)
        {
            var bar = e.GetPosition(ImageControl);
            Global.beginCursorPos.x = bar.X;
            Global.beginCursorPos.y = bar.Y;
            Global.isSelectAreaMove = true;
        }

        private void SelectArea_MouseLeave(object sender, MouseEventArgs e)
        {
            Global.isSelectAreaMove = false;
        }

        private void SelectArea_MouseRelease(object sender, MouseEventArgs e)
        {
            Global.isSelectAreaMove = false;
        }

        public Rect GetOriginRect()
        {
            Point a = new Point();
            a.x = DragBox.Margin.Left;
            a.y = DragBox.Margin.Top;
            Point b = new Point();
            b.x = DragBox.Margin.Left + DragBox.Width;
            b.y = DragBox.Margin.Top + DragBox.Height;
            Point a2 = new Point();
            double d = (Content.Height - Global.mapped_height)/2;
            a2.x = (Global.currentImage.width * a.x) / (Content.Width);
            a2.y = (Global.currentImage.height * (a.y - d)) / Global.mapped_height;
            Point b2 = new Point();
            b2.x = (Global.currentImage.width * b.x) / (Content.Width);
            b2.y = (Global.currentImage.height * (b.y - d)) / Global.mapped_height;
            Rect rect = new Rect();
            rect.left = a2.x;
            rect.top  = a2.y;
            rect.right = b2.x;
            rect.bottom = b2.y;
            //rect.right = Global.currentImage.width - b2.x;
            //rect.bottom = Global.currentImage.height - b2.y;
            return rect;
        }

        private void OnClickZoom(object sender, RoutedEventArgs e)
        {
            Rect rect = GetOriginRect();
            DetailView view = new DetailView();
            view.Show();
            //view.ImageContainer.Width = rect.right - rect.left;
            //view.ImageContainer.Height = rect.bottom - rect.top;
            view.Image.Source = Utility.MakeBitmapSourceFromRegionOfLayer(0, rect);
        }

        private void LeftTopBox_MouseDown(object sender, MouseButtonEventArgs e)
        {
            Global.isSelectAreaResizing = true;
            Global.sideboxID = SideBoxID.LeftTop;
            var bar = e.GetPosition(ImageControl);
            Global.beginCursorPos.x = bar.X;
            Global.beginCursorPos.y = bar.Y;
        }

        private void LeftBotBox_MouseDown(object sender, MouseButtonEventArgs e)
        {
            Global.isSelectAreaResizing = true;
            Global.sideboxID = SideBoxID.LeftBottom;
            var bar = e.GetPosition(ImageControl);
            Global.beginCursorPos.x = bar.X;
            Global.beginCursorPos.y = bar.Y;
        }

        private void Content_MouseMove(object sender, MouseEventArgs e)
        {
            if (Global.isSelectAreaResizing == false)
                return;
            var current_pos = e.GetPosition(ImageControl);
            Point diff = new Point();
            diff.x = Global.beginCursorPos.x - current_pos.X;
            diff.y = Global.beginCursorPos.y - current_pos.Y;
            Global.beginCursorPos.x = current_pos.X;
            Global.beginCursorPos.y = current_pos.Y;
            
            if(Global.sideboxID == SideBoxID.LeftTop)
            {
                double nWidth = DragBox.Width + diff.x;
                double nHeight = DragBox.Height + diff.y;
                if ((nWidth > Constants.DRAG_BOX_MAXIMUM_WIDTH || nWidth < Constants.DRAG_BOX_MINIMUM_WIDTH)
                    || (nHeight > Constants.DRAG_BOX_MAXIMUM_HEIGHT || nHeight < Constants.DRAG_BOX_MINIMUM_HEIGHT))
                    return;
                DragBox.Width = Math.Max(Math.Min(nWidth, Constants.DRAG_BOX_MAXIMUM_WIDTH), Constants.DRAG_BOX_MINIMUM_WIDTH);
                DragBox.Height = Math.Max(Math.Min(nHeight, Constants.DRAG_BOX_MAXIMUM_HEIGHT), Constants.DRAG_BOX_MINIMUM_HEIGHT);
                DragBox.Margin = new Thickness(Math.Max(Math.Min(DragBox.Margin.Left - diff.x, Global.bound.topRight.x - DragBox.Width), 0),
                                               Math.Max(Math.Min(DragBox.Margin.Top - diff.y, (Global.mapped_height + Global.bound.topRight.y) - DragBox.Height), 0 + Global.bound.topRight.y),
                                               DragBox.Margin.Right,
                                               DragBox.Margin.Bottom
                                               );
            }
            else if (Global.sideboxID == SideBoxID.LeftBottom)
            {
                double nWidth = DragBox.Width + diff.x;
                double nHeight = DragBox.Height - diff.y;
                if ((nWidth > Constants.DRAG_BOX_MAXIMUM_WIDTH || nWidth < Constants.DRAG_BOX_MINIMUM_WIDTH)
                    || (nHeight > Constants.DRAG_BOX_MAXIMUM_HEIGHT || nHeight < Constants.DRAG_BOX_MINIMUM_HEIGHT))
                    return;
                DragBox.Width = Math.Max(Math.Min(nWidth, Constants.DRAG_BOX_MAXIMUM_WIDTH), Constants.DRAG_BOX_MINIMUM_WIDTH);
                DragBox.Height = Math.Max(Math.Min(nHeight, Constants.DRAG_BOX_MAXIMUM_HEIGHT), Constants.DRAG_BOX_MINIMUM_HEIGHT);
                DragBox.Margin = new Thickness(Math.Max(Math.Min(DragBox.Margin.Left - diff.x, Global.bound.topRight.x - DragBox.Width), 0),
                                               DragBox.Margin.Top,
                                               DragBox.Margin.Right,
                                               Math.Max(Math.Min(DragBox.Margin.Bottom + diff.y, (Global.mapped_height + Global.bound.topRight.y) - DragBox.Height), 0 + Global.bound.topRight.y)
                                               );
            }
            else if (Global.sideboxID == SideBoxID.RightTop)
            {
                double nWidth = DragBox.Width - diff.x;
                double nHeight = DragBox.Height + diff.y;
                if ((nWidth > Constants.DRAG_BOX_MAXIMUM_WIDTH || nWidth < Constants.DRAG_BOX_MINIMUM_WIDTH)
                    || (nHeight > Constants.DRAG_BOX_MAXIMUM_HEIGHT || nHeight < Constants.DRAG_BOX_MINIMUM_HEIGHT))
                    return;
                DragBox.Width = Math.Max(Math.Min(nWidth, Constants.DRAG_BOX_MAXIMUM_WIDTH), Constants.DRAG_BOX_MINIMUM_WIDTH);
                DragBox.Height = Math.Max(Math.Min(nHeight, Constants.DRAG_BOX_MAXIMUM_HEIGHT), Constants.DRAG_BOX_MINIMUM_HEIGHT);
                DragBox.Margin = new Thickness(DragBox.Margin.Left,
                                               Math.Max(Math.Min(DragBox.Margin.Top - diff.y, (Global.mapped_height + Global.bound.topRight.y) - DragBox.Height), 0 + Global.bound.topRight.y),                                               
                                               Math.Max(Math.Min(DragBox.Margin.Right + diff.x, Global.bound.topRight.x - DragBox.Width), 0),
                                               DragBox.Margin.Bottom
                                               );
            }
            else if (Global.sideboxID == SideBoxID.RightBottom)
            {
                double nWidth = DragBox.Width - diff.x;
                double nHeight = DragBox.Height - diff.y;
                if ((nWidth > Constants.DRAG_BOX_MAXIMUM_WIDTH || nWidth < Constants.DRAG_BOX_MINIMUM_WIDTH)
                    || (nHeight > Constants.DRAG_BOX_MAXIMUM_HEIGHT || nHeight < Constants.DRAG_BOX_MINIMUM_HEIGHT))
                    return;
                DragBox.Width = Math.Max(Math.Min(nWidth, Constants.DRAG_BOX_MAXIMUM_WIDTH), Constants.DRAG_BOX_MINIMUM_WIDTH);
                DragBox.Height = Math.Max(Math.Min(nHeight, Constants.DRAG_BOX_MAXIMUM_HEIGHT), Constants.DRAG_BOX_MINIMUM_HEIGHT);
                DragBox.Margin = new Thickness(DragBox.Margin.Left,
                                               DragBox.Margin.Top,
                                               Math.Max(Math.Min(DragBox.Margin.Right + diff.x, Global.bound.topRight.x - DragBox.Width), 0),
                                               Math.Max(Math.Min(DragBox.Margin.Bottom + diff.y, (Global.mapped_height + Global.bound.topRight.y) - DragBox.Height), 0 + Global.bound.topRight.y)
                                               );
            }
        }

        private void Content_MouseUp(object sender, MouseButtonEventArgs e)
        {
            Global.isSelectAreaResizing = false;
        }

        private void Content_MouseLeave(object sender, MouseEventArgs e)
        {
            Global.isSelectAreaResizing = false;
        }

        private void RightTopBox_MouseDown(object sender, MouseButtonEventArgs e)
        {
            Global.isSelectAreaResizing = true;
            Global.sideboxID = SideBoxID.RightTop;
            var bar = e.GetPosition(ImageControl);
            Global.beginCursorPos.x = bar.X;
            Global.beginCursorPos.y = bar.Y;
        }

        private void RightBotBox_MouseDown(object sender, MouseButtonEventArgs e)
        {
            Global.isSelectAreaResizing = true;
            Global.sideboxID = SideBoxID.RightBottom;
            var bar = e.GetPosition(ImageControl);
            Global.beginCursorPos.x = bar.X;
            Global.beginCursorPos.y = bar.Y;
        }
    }
}
