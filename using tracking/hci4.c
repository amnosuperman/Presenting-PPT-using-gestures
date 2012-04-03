#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include "cv.h"
#include "cvaux.h"
#include "cxcore.h"
#include "highgui.h"
#include "cxmisc.h"
//#include "ml.h"
#include <math.h>
#include "handdet.h"
//#include "tracker.h"
#include "mouse.h"
#include "bg.h"  
#include "handdet.c"
#include "tracker.cpp"
#include "eigenface.c"
#include "facedetect.h"
//#include "mouse.c"

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

    IplImage* temp=cvCreateImage(cvSize(80,120),8,3);
	IplImage* pframe1;
	
	CvRect *pHandRect=0,*vrect=NULL;
	capture=cvCaptureFromCAM(0);	
	if( !initAll() ) exitProgram(-1);
	int g;
	piframe=cvQueryFrame(capture);
	codeBook *c=(codeBook *)malloc(640*480*sizeof(codeBook));

    // Starting to learn the background
    printf("Learning the background... Please wait\n");
    startlearnbackground(capture,c);

	// Main loop
	int counter=0;
	char r;
	IplImage* bgsub_img=NULL;
	bgsub_img=cvCreateImage(cvGetSize(piframe),8,3);
			
			
			
		
		

	pframe=invert(piframe);
	pframe1=cvCloneImage(piframe);
	// Capture and display video frames until a hand
	// is detected
	int i=0;
	//char ch;	
	//char* path="/home/vignesh/imgproc/hand.xml";
	
	while( 1 )
	{		
		counter++;
		
		// Updating the background at regular intervals
		if(counter==10000)
		{
			updatebackground(capture,c);
			counter=0;
		}
		// Look for a hand in the next video frame
		pframe=cvQueryFrame(capture);
		pframe1=cvCloneImage(pframe);
    	        detect_and_draw(pframe);
		//pframe=invert(piframe);		
		pHandRect = detectHand(pframe);
		if(pHandRect)
		{	
			cvRectangle(pframe1,cvPoint((pHandRect->x-4),pHandRect->y-4),cvPoint((pHandRect->x+pHandRect->width+4),pHandRect->y+pHandRect->height+4),CV_RGB(255,0,0),1,8,0);		
		}

		// Show the display image
		cvShowImage( DISPLAY_WINDOW, pframe1 );
		ch=cvWaitKey(10); 
		if(ch==27)
        {   
        	exitProgram(0);
		}
		if(ch=='s')
			// exit loop when a hand is detected
			if(pHandRect) {
				prevx=pHandRect->x;
				prevy=pHandRect->y+pHandRect->height;
				flag=3;
				break;
			}
	}
//	cvWaitKey(0);
	// initialize tracking
	KalmanFilter kfilter;
	startTracking(pframe, *pHandRect,kfilter);
    char ch;
	// Track the detected hand using CamShift
	initPCA();
	counter=0;
	while( 1 )
	{
		counter++;
		
		// Updating the background at regular intervals
		if(counter==10000)
		{
			updatebackground(capture,c);
			counter=0;
		}
		CvRect handBox;

		// get the next video frame
		pframe=cvQueryFrame(capture);
// Subtract the background from the image
		bgsub_img=diff(c,pframe);
		pframe1=cvCloneImage(pframe);
    	detect_and_draw(bgsub_img);
		//pframe=invert(piframe);
		// track the hand in the new video frame
		handBox = combi_track(bgsub_img,kfilter);
        CvRect predrect1=kfilter.predictionReport(prevHandRect);
        CvRect camshiftrect=camshift_track(bgsub_img);
        int old_ht;
		if(!((handBox.x<0)||(handBox.y<0)||((handBox.x+handBox.width)>pframe->width)||((handBox.y+handBox.height)>pframe->height))) 
        {
            if(handBox.height>(1.3*handBox.width))
            {
                old_ht=handBox.height;
                handBox.height=2.4*handBox.width;
                handBox.y-=handBox.height-old_ht;
            }
            cvSetImageROI(pframe,handBox);
//            printf("hi");
//                        cvNamedWindow("temp",1);
            IplImage* temp=cvCreateImage(cvGetSize(pframe),8,3);

            cvCopy(pframe,temp,NULL);
//                                    cvShowImage("temp",pframe);
//                                    cvWaitKey(0);

	        int a;
	        a=recognize(temp);
	        if(handBox.height>(2.3*handBox.width))
            {	
            	if(a==3)
            		a=5;
            }

	        if(a==3)
	        {
	       		if(flag==4)
	       			clickrelease(display,1);
	       		if(flag==5)
	       			clickrelease(display,3);
	       		flag=3;
	       	}

	        else if(a==4)
	        {
	       		if(flag==3)
	       			click(display,1);
	       		if(flag==5)
	       		{	
	       			clickrelease(display,3);
	       			click(display,1);
	       		}
	       		flag=4;
	       	}

	        else if(a==5)
	        {
	       		if(flag==3)
	       			click(display,3);
	       		if(flag==4)
	       		{	
	       			clickrelease(display,1);
	       			click(display,3);
	       		}
	       		flag=5;
	       	}

			diffx=handBox.x+(handBox.width/2)-prevx;
			diffy=handBox.y+handBox.height-(handBox.width/2)-prevy;
            
	        move(display,diffx*0.8,diffy*0.8);
	        
			prevx=handBox.x+(handBox.width/2);
			prevy=handBox.y+handBox.height-(handBox.width/2);

	        printf("\nhi %d\n",a);
	        cvResetImageROI(pframe);
    		cvRectangle(pframe1,cvPoint(handBox.x,handBox.y),cvPoint(handBox.x+handBox.width,handBox.y+handBox.height),CV_RGB(0,0,255),3,8,0);		

        }
//    		cvRectangle(pframe,cvPoint(handBox.x,handBox.y),cvPoint(handBox.x+handBox.width,handBox.y+handBox.height),CV_RGB(0,0,255),3,8,0);		

		//if(!((predrect1.x<0)||(predrect1.y<0)||((predrect1.x+predrect1.width)>pframe->width)||((predrect1.y+predrect1.height)>pframe->height))) 
    		//cvRectangle(pframe,cvPoint(predrect1.x,predrect1.y),cvPoint(predrect1.x+predrect1.width,predrect1.y+predrect1.height),CV_RGB(0,255,0),3,8,0);		

		//if(!((camshiftrect.x<0)||(camshiftrect.y<0)||((camshiftrect.x+camshiftrect.width)>pframe->width)||((camshiftrect.y+camshiftrect.height)>pframe->height))) 
    		//cvRectangle(pframe,cvPoint(camshiftrect.x,camshiftrect.y),cvPoint(camshiftrect.x+camshiftrect.width,camshiftrect.y+camshiftrect.height),CV_RGB(255,255,0),3,8,0);

		cvShowImage( DISPLAY_WINDOW, pframe1 );
		//delete handBox;

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
	initHandDet("/home/vignesh/imgproc/hand.xml");
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
	//free(c);
	exit(code);
}

