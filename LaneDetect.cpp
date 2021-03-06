#include "opencv2/highgui/highgui.hpp"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <raspicam/raspicam_cv.h>
#include "linefinder.h"

#include <wiringPi.h>
#include <softPwm.h>

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define PI 3.1415926

#define LEFT 3
#define RIGHT 0
#define GND1 12
#define GND2 15

using namespace cv;
using namespace std;

int x = 320, y = 240;
int start_flag = 0;
int speed=0;
int del=0;

double leftCorrection = 0;
double rightCorrection = 0;

pthread_mutex_t a_mutex=PTHREAD_MUTEX_INITIALIZER;



void *t_func(void *data)
{

//int speedLeft = 0;
//	int speedRight = 0;

#define NORMAL 75
#define TURN_HIGH 75
#define TURN_LOW 55 
#define STD_X 320
#define STD_Y 240
#define OFFSET 50

#define S_NORMAL 70
#define S_HIGH 90
#define S_LOW 50

#define T_HIGH 100
#define T_LOW 50

#define MAX 640
	double cc=0;
	while(1)
	{
		if(start_flag)
		{
			
			
			if((x <= STD_X + OFFSET)&&(x >= STD_X - OFFSET))
			{
				cout<<"straight "<<endl;
				softPwmWrite(LEFT, NORMAL+leftCorrection);
				softPwmWrite(RIGHT, NORMAL+rightCorrection);
				softPwmWrite(GND1,0);
				softPwmWrite(GND2,0);

				if(leftCorrection > 20)
					leftCorrection = 20;
				if(rightCorrection > 20)
					rightCorrection = 20;

				leftCorrection = (leftCorrection > 0) ? leftCorrection - 2 : 0 ;
				rightCorrection = (rightCorrection > 0 ) ? rightCorrection -2 : 0;
/*				delay(speed/2);
				softPwmWrite(LEFT, 0);
				softPwmWrite(RIGHT, 0);
				delay(del);*/
			}

			else if((x > STD_X + (OFFSET)) && (x<STD_X+(3*OFFSET))) //level 1 right turn
			{
				cout<<"right"<<endl;
				softPwmWrite(LEFT, TURN_HIGH );
				softPwmWrite(RIGHT, TURN_LOW);
				softPwmWrite(GND1,0);
				softPwmWrite(GND2,0);
				delay(speed);
				softPwmWrite(LEFT, 0);
				softPwmWrite(RIGHT, 0);
				delay(del);

				rightCorrection += 0.25;
			}
			else if((x>=STD_X +(3*OFFSET)) && ( x < 600)) // level 2 right turn
			{
				softPwmWrite(LEFT, TURN_HIGH + 8);
				softPwmWrite(RIGHT, TURN_LOW);
				softPwmWrite(GND1,0);
				softPwmWrite(GND2,0);
				delay(speed);
				softPwmWrite(LEFT, 0);
				softPwmWrite(RIGHT, 0);
				delay(del);

				rightCorrection += 0.35;
				
			}
			else if( (x >=600 ) && ( x <=640) ) // level 3 right turn
			{
				softPwmWrite(LEFT, TURN_HIGH + 20);
				softPwmWrite(RIGHT, TURN_LOW + 5);
				softPwmWrite(GND1,0);
				softPwmWrite(GND2,0);
				delay(speed/2);
				softPwmWrite(LEFT, 0);
				softPwmWrite(RIGHT, 0);
				delay(del);

				rightCorrection += 0.8;
				
			}
			else if((x < (STD_X - OFFSET)) && (x>(STD_X -(3*OFFSET)))) // level 1 left turn
			{
				softPwmWrite(LEFT, TURN_LOW);
				softPwmWrite(RIGHT, TURN_HIGH);
				softPwmWrite(GND1,0);
				softPwmWrite(GND2,0);
				delay(speed);
				softPwmWrite(LEFT, 0);
				softPwmWrite(RIGHT, 0);
				delay(del);

				leftCorrection += 0.25;

			}
			else if((x<=(STD_X-(3*OFFSET))) && (x>=40)) // level 2 left turn
			{
				cout<<"left"<<endl;
				softPwmWrite(LEFT, TURN_LOW);
				softPwmWrite(RIGHT, TURN_HIGH + 8);
				softPwmWrite(GND1,0);
				softPwmWrite(GND2,0);
				delay(speed);
				softPwmWrite(LEFT, 0);
				softPwmWrite(RIGHT, 0);
				delay(del);

				leftCorrection += 0.35;
			}
			else if( (x<=40) && (x>=0) ) // level 3 left turn
			{
				cout<<"left"<<endl;
				softPwmWrite(LEFT, TURN_LOW + 5);
				softPwmWrite(RIGHT, TURN_HIGH + 20);
				softPwmWrite(GND1,0);
				softPwmWrite(GND2,0);
				delay(speed/2);
				softPwmWrite(LEFT, 0);
				softPwmWrite(RIGHT, 0);
				delay(del);

				leftCorrection += 0.8;
			}
			/*
			cc = (x-320) / 320.0;
			speedLeft = 65 + 15 * cc;
			speedRight = 50 + -(15) * cc;
			softPwmWrite(LEFT,  speedLeft);
			cout << "left : " << speedLeft << endl;
			softPwmWrite(RIGHT, speedRight );
			cout << "right : " << speedRight  << endl;
			delay(speed);
			*/
		}
	}
}

int getAVG(int buffer[], int size)
{
	int i=0;
	int sum=0;
	for(i=0;i<size;i++)
	{
		sum+=buffer[i];
	}
	return sum/size;
}
int main(int argc, char* argv[]) {
	raspicam::RaspiCam_Cv Camera;
	int count=1;
	int houghVote = 200;
	bool showSteps =true;
	int bufferX[5]={0};
	int bufferY[5]={0};
	speed=atoi(argv[2]);
	del=atoi(argv[3]);
	pthread_t p_thread;
	int thd_id;

	if(wiringPiSetup() == -1)
	{
		cout<<"setup error"<<endl;
		return -1;
	}

	//led output
	pinMode(4, OUTPUT);

	if(atoi(argv[1]) == 1)
	{
		//	pthread_mutex_init(&a_mutex,NULL);
		//       pin setting
		pinMode(LEFT, OUTPUT);
		pinMode(RIGHT, OUTPUT);
		pinMode(GND1, OUTPUT);
		pinMode(GND2, OUTPUT);
		softPwmCreate(LEFT, NORMAL, 200);
		softPwmCreate(RIGHT, NORMAL, 200);

		//	system("./tset 0");
		thd_id = pthread_create(&p_thread, NULL, t_func, NULL);
	}
	//	pthread_join(p_thread, NULL);
	//VideoCapture capture(argv[1]);
	Camera.set(CV_CAP_PROP_FORMAT, CV_8UC1);
	Camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	//Open camera
	if (!Camera.open()) {cerr<<"Error opening the camera"<<endl;return -1;}

//	namedWindow("image",WINDOW_NORMAL);    
//	resizeWindow("image",640,480);
	Mat image;
	Mat gray;
	LineFinder ld;

	while(1)
	{
		//capture>>image;
		Camera.grab();
		Camera.retrieve(image);
		if(image.empty())
			break;
		//                cvtColor(image,gray,CV_RGB2GRAY);
		Rect roi(0,(image.rows*2)/3,image.cols-1,image.rows/3-1);// set the ROI for the image
		Mat imgROI;//
		GaussianBlur(image(roi),imgROI,Size(3,3),0);
		// Display the image

		// Canny algorithm
		Mat contours;
		Canny(imgROI,contours,75,140);
		imshow("display",contours);
		waitKey(1);
		// Detect lines
		ld.setLineLengthAndGap(60,10);
		ld.setMinVote(4);
		ld.findLines(contours);
#if DEBUG == 1
		cout<<"after findlines"<<endl;
#endif
		//	    std::vector<Vec4i> li= ld.findLines(contours);

		ld.processSide();
#if DEBUG == 1
		cout<<"after processSide"<<endl;
#endif
		ld.laneFilter();
#if DEBUG == 1
		cout<<"after laneFilter"<<endl;
#endif

		ld.calcIntersectP();
#if DEBUG == 1
		cout<<"after calcIntersectP "<<endl;
#endif
		//this intersection has the vanishing point
		//intersection has (x,y) coordinates access it like following
		//intersection.x
		//intersection.y
		Point intersection=ld.getIntersectP();

		//		pthread_mutex_lock(&a_mutex);
		start_flag = 1;
		bufferX[count%1]=intersection.x;
		bufferY[count%1]=intersection.y;
		count++;
		x=getAVG(bufferX,1);//+bufferX[3]+bufferX[4])/5;//(x+intersection.x)/count;
		y=getAVG(bufferY,1);//+bufferY[3]+bufferY[4])/5;    //(y+intersection.y)/count;
		//		pthread_mutex_unlock(&a_mutex);

		ld.setShift((image.rows*2)/3);
		ld.drawLines(image);
		line(image, Point(320-OFFSET,0),Point(320-OFFSET,480),Scalar(255),3);
		line(image, Point(320+OFFSET,0),Point(320+OFFSET,480),Scalar(255),3);

#if DEBUG == 1
		cout<<"After drawLines"<<endl;
#endif

		if(showSteps){
			imshow("image",image);
			waitKey(1);
		}
		//	    ld.drawDetectedLines(image);
		//		udelay(1000);
		//		cout<<"in main"<<endl;
	}
	char key = (char) waitKey(0);

	pthread_join(p_thread, NULL);
	//	pthread_mutex_destroy(&a_mutex);
}




