// uEyeCaptureSingle.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
        #pragma comment (lib, "winmm.lib")
	#include <windows.h>
	#include <mmsystem.h>
	#include <direct.h>
	#include "opencv2\opencv.hpp"
	#include "opencv2\highgui\highgui.hpp"
	#include <crtdbg.h>
        #include <chrono>
#else
   #include <opencv/cxcore.h>
   #include <opencv2/opencv.hpp>
   #include <opencv2/highgui/highgui.hpp>
   #include <sys/stat.h>

   typedef int INT;

   typedef struct {
     int wMonth, wDay, wHour, wMinute, wSecond, wMilliseconds;
     struct timespec ts;
   } SYSTEMTIME;

#endif

#include <sstream>
#include <cstdlib>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#define MAXFRAMES	50
#define D_FRAMES	200 //保存するフレーム数
#define B_FRAMES	100 //imgBufの数

#define CAP_WAIT	10

//
#ifndef _WIN32
void GetLocalTime( SYSTEMTIME *t )
{
  struct tm *tm;
  char timbuf[64];

  clock_gettime(CLOCK_REALTIME, &t->ts);
  time_t tim = t->ts.tv_sec;

  tm = localtime(&tim);

  t->wMonth = tm->tm_mon+1;
  t->wDay = tm->tm_mday;
  t->wHour = tm->tm_hour;
  t->wMinute = tm->tm_min;
  t->wSecond = tm->tm_sec;
  t->wMilliseconds = t->ts.tv_nsec/1000;
}
#endif

void cvSaveImageSub(const char path[256],const IplImage *ImageBuf,INT nSizeX,INT nSizeY,int nCaptured)
{
	IplImage *view;
	char fnamebuff[256];

#ifdef _WIN32
	sprintf(fnamebuff,"%s\\%04d.png",path, nCaptured);
#else
	sprintf(fnamebuff,"%s/%04d.png",path, nCaptured);
#endif
	view = cvCreateImage( cvSize(ImageBuf->width/2,ImageBuf->height/2),IPL_DEPTH_8U, 3);
	cvResize(ImageBuf,view,CV_INTER_NN);
	cvSaveImage(fnamebuff,view);
	cvReleaseImage(&view);
}


void MKDIR( char *path )
{
#ifdef _WIN32
	mkdir(path);
#else
	mkdir(path,S_IRUSR | S_IWUSR | S_IXUSR |         /* rwx */
               S_IRGRP | S_IWGRP | S_IXGRP |         /* rwx */
	      S_IROTH | S_IXOTH | S_IXOTH);
#endif
}


int main(int argc, char * argv[])
{
	IplImage    *imgBuf[B_FRAMES],*imgBuf2[B_FRAMES];
	IplImage	*view,*view2;
	IplImage	*show_view,*show_view2;
	time_t		ti, ti2;
	int			nCaptured = 0;
	int			ndev = 1, dev1 = 0, dev2 = 1;		// device number
	struct tm	*lt;
	char		path[256], fname[256];

//	ostringstream obuff;
	SYSTEMTIME shoot_timelist[1000];

	std::string		c_type, c_type2;
	boost::thread_group thr_grp; 

	// opencv camera
    cv::VideoCapture cap1, cap2; // デフォルトカメラをオープン

	cv::Mat	 framebuf[100];
	IplImage imbuf[100];

	// check arguments
	if( argc == 2 ){
		if( strlen(argv[1]) == 6 ){
			if( strncmp(argv[1],"-dev=",5) == 0 ){
				ndev = 1;
				dev1 = argv[1][5] - '0';
				printf("device number = %d\n", dev1);
			}
		} else {
			if( strncmp(argv[1],"-dev=",5) == 0 )
			{
				ndev = 2;
				dev1 = argv[1][5] - '0';
				dev2 = argv[1][6] - '0';
				printf("device number = %d, %d\n", dev1, dev2);
			}
		}
	}

	cap1.open(dev1);
	//cap2.open(dev2);

    //if(!cap1.isOpened() || !cap2.isOpened()){
	if(!cap1.isOpened()){
		// 成功したかどうかをチェック
		printf("cannot open cameras.\n");
        return -1;
	}

    cv::Mat input_image;
    cv::namedWindow("cap1",1);

    printf("Stop [ESC], Start capture = space\n");

    for(;;)
    {
	cv::Mat frame1, frame2, frame3;
        cap1 >> frame1;

//		frame3 = cv::Mat(cv::Size(frame1.cols + frame2.cols,frame1.rows),frame1.type());

//		cv::Mat roi(frame3, cv::Rect(0,0,frame1.cols,frame1.rows));
//		frame1.copyTo(roi);
//		roi = cv::Mat(frame3, cv::Rect(frame1.cols,0,frame1.cols,frame1.rows));
//		frame2.copyTo(roi);

        imshow("cap1", frame1);

	char k = cv::waitKey(10);
	if( k == 0x1b )
	  exit(1);
	if( k == ' ' )
		break;
    }

	printf("now start to capture. [ESC] to quit. \n");

	time(&ti);
	lt = localtime(&ti);

	// create directory
	MKDIR("PupilLabsData");
	sprintf(path,"PupilLabsData/%02d%02d_%02d%02d%02d", 
		lt->tm_mon+1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
	MKDIR(path);

	//make time file
	char fname_tx[256];
	FILE *outputfilex;
	int Twrote = 0;
	int Written = 0;

#ifdef _WIN32
	sprintf(fname_tx, "%s\\timex.csv", path);
#else
	sprintf(fname_tx, "%s/timex.csv", path);
#endif
	outputfilex = fopen(fname_tx, "w");
	if (outputfilex == NULL){
		printf("cannot make time.csv\n");
	}

	// image saving
    for(;;){
		char key;

		for(int nbuf = 0; nbuf < 100; nbuf++ ){
			cv::Mat frame1, frame2, frame3;

			cap1 >> frame1;

//			frame3 = cv::Mat(cv::Size(frame1.cols + frame2.cols,frame1.rows),frame1.type());

//			cv::Mat roi(frame3, cv::Rect(0,0,frame1.cols,frame1.rows));
//			frame1.copyTo(roi);
//			roi = cv::Mat(frame3, cv::Rect(frame1.cols,0,frame1.cols,frame1.rows));
//			frame2.copyTo(roi);

			framebuf[nbuf] = frame1.clone();

			imbuf[nbuf] = framebuf[nbuf];

			//sTime = timeGetTime();
			//shoot_timelist[nCaptured] = timeGetTime();

			GetLocalTime(&shoot_timelist[Twrote]);
			Twrote++;

			if (((nCaptured + 1) % 1000) == 0){
				for (int i = 0; i < 1000; i++){
#ifdef _WIN32
	fprintf_s(outputfilex, "%d,%02d%02d%02d%02d%02d%04d\n", Written, shoot_timelist[i].wMonth, shoot_timelist[i].wDay, shoot_timelist[i].wHour, shoot_timelist[i].wMinute, shoot_timelist[i].wSecond, shoot_timelist[i].wMilliseconds);
#else
	fprintf(outputfilex, "%d,%02d%02d%02d%02d%02d%04d\n", Written, shoot_timelist[i].wMonth, shoot_timelist[i].wDay, shoot_timelist[i].wHour, shoot_timelist[i].wMinute, shoot_timelist[i].wSecond, shoot_timelist[i].wMilliseconds);
#endif					
					Written = Written + 1;
				}
				Twrote = 0;
			}

			thr_grp.create_thread(boost::bind(&cvSaveImageSub,path,&imbuf[nbuf],0,0,nCaptured));//save thread create
			nCaptured++;

			// show image for every 4 frms
			if( (nCaptured % 10) == 0 ){
				imshow("cap1", framebuf[nbuf]);
				printf("getted %s\\%04d.png\n",path,nCaptured);
			}

			key = cv::waitKey(1);
			if( key == 0x1b )
				break;
		}
		if( key == 0x1b )
			break;
	}

	printf("Finish\nCamera%d:%d Captured\n",dev1, nCaptured);
	thr_grp.join_all();

	//画像の取得時刻(ms)をファイルに出力
	if (Twrote != 0){
		for (int i = 0; i < Twrote; i++){
#ifdef _WIN32
			fprintf_s(outputfilex, "%d,%02d%02d%02d%02d%02d%04d\n", Written, shoot_timelist[i].wMonth, shoot_timelist[i].wDay, shoot_timelist[i].wHour, shoot_timelist[i].wMinute, shoot_timelist[i].wSecond, shoot_timelist[i].wMilliseconds);
#else
		        fprintf(outputfilex, "%d,%02d%02d%02d%02d%02d%04d\n", Written, shoot_timelist[i].wMonth, shoot_timelist[i].wDay, shoot_timelist[i].wHour, shoot_timelist[i].wMinute, shoot_timelist[i].wSecond, shoot_timelist[i].wMilliseconds);
#endif
			Written = Written + 1;
		}
	}
	fclose(outputfilex);

	//各フレームにかかった時間をファイルに出力
	/*
	char	fname_t[256];
	FILE	*outputfile;
	sprintf(fname_t,"%s\\time.csv",path);
	outputfile = fopen(fname_t,"w");
	if(outputfile == NULL){
		printf("cannot make time.csv\n");
	}
	for(int i =0; i < nCaptured; i++){
		fprintf(outputfile,"%d,",timelist[i]);
	}
	fclose(outputfile);
	*/

}
