#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include "cv.h"
#include "cvaux.h"
#include "cxcore.h"
#include "highgui.h"
#include "cxmisc.h"
#include "ml.h"
#include <math.h>

#include "fake.h"

// Move mouse pointer
void move (Display *display, int x, int y)
{
  	XWarpPointer (display, None, None, 0,0,0,0, x, y);
  	XFlush (display);
  	usleep (1);
}
// Get mouse coordinates
void coords (Display *display, int *x, int *y)
{
  	XEvent event;
  	XQueryPointer (display, DefaultRootWindow (display),&event.xbutton.root, &event.xbutton.window,&event.xbutton.x_root, &event.xbutton.y_root,
        &event.xbutton.x, &event.xbutton.y,&event.xbutton.state);
	*x = event.xbutton.x;
	*y = event.xbutton.y;
}
// Simulate mouse click
void
click (Display *display, int button,int flag)
{
  // Create and setting up the event
    int x,y;
    int m,n;
    if(flag==0)
    {
        m=1320;
        n=460;
    }
    else if(flag==1)
    {
        m=15;
        n=460;
    }
    coords(display,&x,&y);
    move(display,m-x,n-y);
  XEvent event;
  memset (&event, 0, sizeof (event));
  event.xbutton.button = button;
  event.xbutton.same_screen = True;
  event.xbutton.subwindow = DefaultRootWindow (display);
  while (event.xbutton.subwindow)
    {
      event.xbutton.window = event.xbutton.subwindow;
      XQueryPointer (display, event.xbutton.window,
&event.xbutton.root, &event.xbutton.subwindow,
&event.xbutton.x_root, &event.xbutton.y_root,
&event.xbutton.x, &event.xbutton.y,
&event.xbutton.state);
    }
//      	event.xbutton.x=1330;
  //  	event.xbutton.y=427;
    //	event.xbutton.x_root=1330;
    	//event.xbutton.y_root=427;

  // Press
  event.type = ButtonPress;
  if (XSendEvent (display, PointerWindow, True, ButtonPressMask, &event) == 0)
    fprintf (stderr, "Error to send the event!\n");
//printf("(%d,%d) (%d,%d)\n",event.xbutton.x,event.xbutton.y,event.xbutton.x_root,event.xbutton.y_root);    
  XFlush (display);
  //usleep (1);
  // Release

  event.type = ButtonRelease;
  if (XSendEvent (display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
    fprintf (stderr, "Error to send the event!\n");
  XFlush (display);
  //usleep (1);
  move(display,x-m,y-n);
}

// Get pixel color at coordinates x,y
/*void pixel_color (Display *display, int x, int y, XColor *color)
{
 	XImage *image;
	image = XGetImage (display, DefaultRootWindow (display), x, y, 1, 1, AllPlanes, XYPixmap);
	color->pixel = XGetPixel (image, 0, 0);
	XFree (image);
	XQueryColor (display, DefaultColormap(display, DefaultScreen (display)), color);
}*/
// START HERE
int main (int argc, char *argv[])
{
	int starting = 3;
	int x = 0,diffx=0,prevx=0;
	int y = 0,diffy=0,prevy=0;
 
  	// Open X display
	Display *display = XOpenDisplay (NULL);
	if (display == NULL)
        {
      		fprintf (stderr, "Can't open display!\n");
      		return -1;
    	}
  
  	// Wait 3 seconds to start
  	printf ("Starting in ");
  		fflush (stdout);
  	while (starting > 0)
    	{
      		printf ("\b\b\b %d...", starting);
      		fflush (stdout);
      		sleep (1);
      		starting--;
    	}
  	printf ("\n");

	//declaration block
	cvNamedWindow("hand");
	int height,width,step,channels;
	int i1,j1,k1;
	IplImage * block1_img_i1 = NULL;
	IplImage * block1_img_o3 = NULL;
	IplImage * img2;
	uchar *data;
	uchar *data2;	
	static CvMemStorage* block1_storage = 0;
	static CvHaarClassifierCascade* block1_cascade = 0;
	const char* block1_cascade_name = "/home/vignesh/Downloads/hand.xml";
	char c;
	//execution block
	CvCapture* block2_capture = NULL; 
	block2_capture = cvCaptureFromCAM(1); 
		block1_img_i1=cvQueryFrame(block2_capture);	
	int m=0;
	double scale=1.3;
	CvPoint initpoint,currpoint;
	int o=50,p=50;
		img2=cvCreateImage(cvGetSize(block1_img_i1),block1_img_i1->depth,3);		
		block1_cascade = (CvHaarClassifierCascade*)cvLoad( block1_cascade_name, 0, 0, 0 );
		printf("hey\n");
		block1_storage = cvCreateMemStorage(0);
		IplImage* gray = cvCreateImage( cvSize(block1_img_i1->width,block1_img_i1->height), 8, 1 );
		IplImage* small_img = cvCreateImage( cvSize( cvRound (block1_img_i1->width/scale), cvRound (block1_img_i1->height/scale)),8, 1 );
		printf("hi\n");
	while(1)
	{
		o++;
		block1_img_i1=cvQueryFrame(block2_capture);	
		height= block1_img_i1->height;
		width= block1_img_i1->width;
		step= block1_img_i1->widthStep;
		channels = block1_img_i1->nChannels;
//		data= (uchar *)block1_img_i1->imageData;
//		data2= (uchar *)img2->imageData;
/*		for(i1=0;i1<height;i1++) 
			for(j1=0;j1<width;j1++) 
				for(k1=0;k1<channels;k1++)
					data2[(i1+1)*step-(j1+1)*channels+k1]=data[i1*step+j1*channels+k1];
*/		
		cvFlip(block1_img_i1,img2,1);
		cvCvtColor( block1_img_i1, gray, CV_BGR2GRAY );
		cvResize( gray, small_img, CV_INTER_LINEAR );
		cvEqualizeHist( small_img, small_img );
		cvClearMemStorage( block1_storage );
		CvSeq* faces = cvHaarDetectObjects( small_img, block1_cascade, block1_storage,2.5, 2.0, 0/*CV_HAAR_DO_CANNY_PRUNING*/,cvSize(24,24) );
//
//		coords (display, &x, &y);
//		printf("%d,%d\n",x,y);
//
		if(faces)
		{
			int i=0;
			CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
			if(r)
			{
				CvPoint center;
				int radius;
				center.x = img2->width-cvRound((r->x + r->width*0.5)*scale);
				center.y = cvRound((r->y + r->height*0.5)*scale);
				//coords (display, &x, &y);
				usleep(100);
				radius = cvRound((r->width + r->height)*0.25*scale);
				cvCircle( img2, center, radius, cvScalarAll(0), 3, 8, 0 );

				if((m==0)/*&&(center.x>(img2->width/3))*/&&(center.x<(2*img2->width/3)))
				{
				    
				    m=1;
				    p=0;
				    initpoint.x=center.x;
				    initpoint.y=center.y;
				    currpoint.x=center.x;
				    currpoint.y=center.y;
				    
				}
				else if(m==1)
				{
				    m=1;
				    p=0;
				    coords (display, &x, &y);
				    currpoint.x=center.x;
				    currpoint.y=center.y;
//				    printf("currpoint : (%d,%d)\n",x,y);
				}
				else
				{
				}
			}
		}
		if(faces->total==0)
		{
		    p++;
		    if(m==1)
		    {
		        m=2;
		        printf("here\n");
		        printf("initpoint : (%d,%d) currpoint : (%d,%d)\n",initpoint.x,initpoint.y,currpoint.x,currpoint.y);
		        if((currpoint.x-initpoint.x)>80)
		        {
		            click(display,1,0);
		            //fake(display, 1);
		            //clickrelease(display,1);
		            m=0;
		            p=0;
		            o=0;
		            printf("right click\n");
		        }
		        else if((initpoint.x-currpoint.x)>80)
		        {
		            //click(display,1,1);
		            fake(display, 0);
		            //clickrelease(display,1);
		            m=0;
		            o=0;
		            p=0;
		            printf("left click\n");
		        }
		        
		    }
		    if(p>=10)
		        m=0;
		}
		z :
		cvShowImage("hand" ,img2);
		c=cvWaitKey(15);
		if(c==27)
			break;
	}

	//deallocation block
		cvReleaseImage( &gray );
		cvReleaseImage( &small_img );

	cvReleaseImage(&block1_img_o3);
	cvReleaseImage(&block1_img_i1);
	cvReleaseMemStorage(&block1_storage);

	XCloseDisplay (display);
	return 0;
}
