// Object Tracking

// Header files required
#include "stdafx.h"
#include <sstream>
#include <string>
#include <iostream>
#include <opencv\highgui.h>
#include "opencv2/highgui/highgui.hpp"
#include <opencv\cv.h>

using namespace cv;
using namespace std;
Mat cameraFeed;
int flag=0;
int startx,starty,endx,endy;
int H_MIN = 255;
int H_MAX = 0;
int S_MIN = 255;
int S_MAX = 0;
int V_MIN = 255;
int V_MAX = 0;

Mat HSV;

void gt(Point p)
{   int width=30,i,j;
	Vec3b color;
	int h,s,v,h1,s1,v1;
	color=HSV.at<Vec3b>(p.y,p.x);
	h=(int)color.val[0];
	s=(int)color.val[1];
	v=(int)color.val[2];
	//startx=p.y-width;endx=p.y+width;starty=p.x-width;endy=p.x+width;
	flag=1;

    H_MIN = h-width;
	H_MAX = h+width;
    S_MIN = s-width;
    S_MAX = s+width;
    V_MIN = v-width;
    V_MAX = v+width;

	
	cout<< H_MAX<<" "<<H_MIN<<" "<<S_MAX<<" "<<S_MIN<<" "<<V_MAX<<" "<<V_MIN;
}


Point p;
void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
     if  ( event == EVENT_LBUTTONDOWN )
     {
          cout << "Click  (" << x << ", " << y << ")" << endl;
		  p=Point(x,y);
		  gt(p);

     }
     
}

//initial min and max HSV filter values.
//these will be changed using trackbars on runtime


//default capture width and height if you increase it than the default values then operation gets slower
const int FRAME_WIDTH = 640;		//640
const int FRAME_HEIGHT = 480;		//480

//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS=50;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;

//names that will appear at the top of each window
const string windowName = "Original Image";
const string windowName1 = "HSV Image";
const string windowName2 = "Thresholded Image";
const string windowName3 = "After Morphological Operations";
const string trackbarWindowName = "Trackbars";


void on_trackbar( int, void* )
{   //This function gets called whenever a
	// trackbar position is changed
    // since the hsv values are declared global this funtion is empty
}

// function to create window for trackbars
void createTrackbars(){
	
    namedWindow(trackbarWindowName,0);
	//create memory to store trackbar name on window

	//create trackbars and insert them into window
	                                     
    createTrackbar( "H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar );
    createTrackbar( "H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar );
    createTrackbar( "S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar );
    createTrackbar( "S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar );
    createTrackbar( "V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar );
    createTrackbar( "V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar );

}

void drawObject(int x,int y, Mat &frame){
// use any opencv drawing functions to draw a shape of your choice that will appear on the detected object

//void circle(Mat& img, Point center, int radius, const Scalar& color, int thickness=1, int lineType=8, int shift=0)

circle(frame,Point(x,y),35,Scalar(0,0,255),3);

//void putText(Mat& img, const string& text, Point org, int fontFace, double fontScale, Scalar color, int thickness=1, int 

//lineType=8, bool bottomLeftOrigin=false 
putText(frame,"Object",Point(x-20,y),FONT_HERSHEY_SIMPLEX,0.8,Scalar(0,0,255),2,8);
 
}

// function for morphological operations like erode and dilate
void morphOps(Mat &thresh){

	//create structuring element that will be used to "dilate" and "erode" image.
	//the element chosen here is a 3*3  rectangle

	Mat erodeElement = getStructuringElement( MORPH_RECT,Size(3,3));
    //dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement( MORPH_RECT,Size(8,8));
	//erode and dilate the image twice
	erode(thresh,thresh,erodeElement);
	erode(thresh,thresh,erodeElement);


	dilate(thresh,thresh,dilateElement);
	dilate(thresh,thresh,dilateElement);
	
}

void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed){

	Mat temp;
	threshold.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp,contours,hierarchy,CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE );
	//use moments method to find our filtered object
	double refArea = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();
        //if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
        if(numObjects<MAX_NUM_OBJECTS){
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;

				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we safe a reference area each
				//iteration and compare it to the area in the next iteration.
                if(area>MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea){
					x = moment.m10/area;
					y = moment.m01/area;
					objectFound = true;
					refArea = area;
				}else objectFound = false;


			}
			//let user know you found an object
			if(objectFound ==true){
				putText(cameraFeed,"Tracking Object",Point(0,20),1,1,Scalar(0,0,255),2);
				//draw object location on screen
				drawObject(x,y,cameraFeed);}

		}else putText(cameraFeed,"TOO MUCH NOISE! ADJUST FILTER",Point(0,50),1,2,Scalar(0,0,255),2);
	}
}
int main(int argc, char* argv[])
{

	//some boolean variables for different functionality within this program
    bool trackObjects = true;
    bool useMorphOps = true;
	//Matrix to store each frame of the webcam feed
	
	//matrix storage for HSV image
	
	//matrix storage for binary threshold image
	Mat threshold;
	//x and y values for the location of the object
	int x=0, y=0;
	
	//video capture object to acquire webcam feed
	VideoCapture capture(0);
	//open capture object at location zero (default location for webcam)
	capture.open(0);
	//set height and width of capture frame
	capture.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
	//start an infinite loop where webcam feed is copied to cameraFeed matrix
	//all of our operations will be performed within this loop
	

	//create slider bars for HSV filtering
	//createTrackbars();
	
	
	while(1){
		
		//store image to matrix
		capture.read(cameraFeed);
		//convert frame from BGR to HSV colorspace
		cvtColor(cameraFeed,HSV,COLOR_BGR2HSV);
		//filter HSV image between values and store filtered image to threshold matrix
		
		setMouseCallback(windowName, CallBackFunc, NULL);
		inRange(HSV,Scalar(H_MIN,S_MIN,V_MIN),Scalar(H_MAX,S_MAX,V_MAX),threshold);
		//perform morphological operations on thresholded image to eliminate noise
	
		if(useMorphOps)
		morphOps(threshold);
		//pass in thresholded frame to our object tracking function
		//this function will return the x and y coordinates of the
		//filtered object
		if(trackObjects)
			trackFilteredObject(x,y,threshold,cameraFeed);
		if(flag==1)
		rectangle(cameraFeed, Point(starty,startx), Point (endy,endx),Scalar(0,0,0),5,8,0);
		//show frames 
		imshow(windowName2,threshold);
		imshow(windowName,cameraFeed);
		//imshow(windowName1,HSV);
		

		//delay 30ms so that screen can refresh.
		//image will not appear without this waitKey() command
		waitKey(30);
	}

	return 0;
}
