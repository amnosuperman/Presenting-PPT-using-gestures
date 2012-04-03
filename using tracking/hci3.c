#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include "cv.h"
#include "cvaux.h"
#include "cxcore.h"
#include "highgui.h"
#include "cxmisc.h"
#include <math.h>
#include "handdet.h"
#include "handdet.c"
#include "tracker.cpp"
#include "eigenface.c"
#include "facedetect.c"
#include "text.c"
#include "mouse.c"
#include "fake.cpp"

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

//// Constants
const char * DISPLAY_WINDOW = "DisplayWindow";
//// Global variables
IplImage  * pframe = 0,*piframe;
CvCapture *capture;
//// Function definitions
int initAll();
void exitProgram(int code);

IplImage* invert(IplImage* img)
{
	int height,width,step,channels;
	int i1,j1,k1;
	uchar *data,*data2;
	IplImage * img2 = NULL;
	img2=cvCreateImage(cvGetSize(img),img->depth,3);		
	height= img->height;
	width=img->width;
	step= img->widthStep;
	channels = img->nChannels;
	data= (uchar *)img->imageData;
	data2= (uchar *)img2->imageData;
	for(i1=0;i1<height;i1++) 
		for(j1=0;j1<width;j1++) 
			for(k1=0;k1<channels;k1++)
				data2[(i1+1)*step-(j1+1)*channels+k1]=data[i1*step+j1*channels+k1];
	return img2;
}	

// main()
int main( int argc, char** argv )
{

	int starting = 3;
	int flag=0;
	CvPoint pt;
	int x = 0,diffx=0,prevx=0,initx=0;
	int y = 0,diffy=0,prevy=0,inity=0;
 
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
    IplImage* temp=cvCreateImage(cvSize(80,120),8,3);
	IplImage* pframe1;
	
	CvRect *pHandRect=0,*vrect=NULL;
	capture=cvCaptureFromCAM(0);	
	if( !initAll() ) exitProgram(-1);
	
		int g;
	piframe=cvQueryFrame(capture);
	pframe=invert(piframe);
	pframe1=cvCloneImage(piframe);
	// Capture and display video frames until a hand
	// is detected
	int i=0;
	char c;	
	initPCA();
    char ch;

	x :
	printf("came to x\n");
	while( 1 )
	{		
		// Look for a hand in the next video frame
		pframe=cvQueryFrame(capture);
		pframe1=cvCloneImage(pframe);
    	detect_and_draw(pframe);
		pHandRect = detectHand(pframe);
		
		if((pHandRect)&&(pHandRect->x>4)&&(pHandRect->y>4)&&(pHandRect->x*pHandRect->y<(240*300))&&(pHandRect->x<630)&&(pHandRect->y<470))
		{	
			cvRectangle(pframe1,cvPoint((pHandRect->x-4),pHandRect->y-4),cvPoint((pHandRect->x+pHandRect->width+4),pHandRect->y+pHandRect->height+4),CV_RGB(255,0,0),1,8,0);		
			i++;
		}
		else 
			i=0;
		// Show the display image
		cvShowImage( DISPLAY_WINDOW, pframe1 );
		cvMoveWindow(DISPLAY_WINDOW,0,0);
		c=cvWaitKey(10); 
		if(c==27)
                {   
        	exitProgram(0);
		}
		if(i>=3)
		{	// exit loop when a hand is detected
			if(pHandRect) {
				i=0;
				prevx=pHandRect->x;
				initx=pHandRect->x;
				prevy=pHandRect->y+pHandRect->height;
				flag=3;
				break;
			}
		}
	}

	// initialize tracking
	KalmanFilter kfilter;
	startTracking(pframe, *pHandRect,kfilter);
	// Track the detected hand using CamShift
	while( 1 )
	{
		CvRect handBox;

		// get the next video frame
		pframe=cvQueryFrame(capture);
		pframe1=cvCloneImage(pframe);
		handBox = combi_track(pframe,kfilter);
        int old_ht;
        int a;
		IplImage* temp;
		if(!((handBox.x<0)||(handBox.y<0)||((handBox.x+handBox.width)>pframe->width)||((handBox.y+handBox.height)>pframe->height))) 
        {
            if(handBox.height>(1.3*handBox.width))
            {
                old_ht=handBox.height;
                handBox.height=2.4*handBox.width;
                handBox.y-=handBox.height-old_ht;
            }
            cvSetImageROI(pframe,handBox);
            temp=cvCreateImage(cvGetSize(pframe),8,3);

            cvCopy(pframe,temp,NULL);

	        a=recognize(temp);
	        cvReleaseImage(&temp);
	        if(handBox.height>(2.3*handBox.width))
            {	
            	if(a==3)
            		a=5;
            }
			diffx=handBox.x+(handBox.width/2)-initx;
			diffy=handBox.y+handBox.height-(handBox.width/2)-prevy;
			prevx=handBox.x+(handBox.width/2);
			prevy=handBox.y+handBox.height-(handBox.width/2);

	        cvResetImageROI(pframe);
    		cvRectangle(pframe1,cvPoint(handBox.x,handBox.y),cvPoint(handBox.x+handBox.width,handBox.y+handBox.height),CV_RGB(0,0,255),3,8,0);		
            
	        if(diffx<(-60))
	        {	click(display,1,0);
	        	printf("right click\n");
	        	goto x;
	        }
	        else if(diffx>(60))
	        {
	        	fake(display, 0);
	        	printf("left click\n");
	        	goto x;
	        }
	        else
	        {}

        }
        else
        	goto x;

		cvShowImage( DISPLAY_WINDOW, pframe1 );

		ch=cvWaitKey(10);
		if( ch==27 ) {
			exitProgram(0);			
			break;
		}
		if(ch=='s'){
		    cvSetImageROI(pframe,handBox);
		    cvResize(pframe,temp);
		    cvSaveImage("image6.jpg",temp);
		    cvResetImageROI(pframe);
		}
	}
	return 0;
}


//////////////////////////////////
// initAll()
//
int initAll()
{
	// Create the display window
	cvNamedWindow( DISPLAY_WINDOW, 1 );
	// Initialize tracker
	pframe=cvQueryFrame(capture);
	if( !createTracker(pframe) ) 
		return 0;
	initHandDet("/home/vignesh/Downloads/hand.xml");
	//initHandDet1("v.xml");
	// Set Camshift parameters
	setVmin(60);
	setSmin(50);
	initface();
	return 1;
}


//////////////////////////////////
// exitProgram()
//
void exitProgram(int code)
{
	// Release resources allocated in this file
	cvDestroyWindow( DISPLAY_WINDOW );
	cvReleaseImage( &pframe );
	releaseface();
	// Release resources allocated in other project files
	cvReleaseCapture(&capture);
	closeHandDet();
	releaseTracker();

	exit(code);
}

