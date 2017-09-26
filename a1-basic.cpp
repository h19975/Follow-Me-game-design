#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string>
#include <sstream>
#include <vector>
#include <sys/time.h>
#include <math.h>

using namespace std;

#include "simon.h"


struct XInfo {
	Display* display;
	int screen;
	Window window;
	GC gc;
	int width;
	int height;
};






class Displayable {
        public:
                virtual void paint(XInfo& xinfo) = 0;
};





vector <Displayable *> d;
bool needspace = false;





class Text : public Displayable {
	public:         
		int x;
                double y;
                string s;
		bool block;
                Text(int x, int y, string t, bool b) {
                        this->x = x;
                        this->y = y;
                        this->s = t;
			this->block = b;
                }
                virtual void paint(XInfo& xinfo) {
			if (!block) {
                        	XDrawImageString(xinfo.display, xinfo.window, xinfo.gc, x, y, s.c_str(), s.length());
			}
                }
};





class Circle : public Displayable {
        public:
                int x;
                double y;
                int diameter;
		int linewidth;
		bool fillBlack;
		int time;
		int interval;
		Circle(int x, int y, int d, int lw, bool fb, int t, int i) {
			this->x = x;
			this->y = y;
			diameter = d;
			linewidth = lw;
			fillBlack = fb;
			time = t;
			interval = i;
		}
                virtual void paint(XInfo& xinfo) {
			XSetLineAttributes(xinfo.display, xinfo.gc, linewidth, LineSolid, CapButt, JoinRound);
			if (fillBlack) {
				XFillArc(xinfo.display, xinfo.window, xinfo.gc, x, y, diameter, diameter, 0, 360*64);
				GC temp = xinfo.gc;
				GC gc1 = XCreateGC(xinfo.display, xinfo.window, 0, 0);
                	        XSetForeground(xinfo.display, gc1, WhitePixel(xinfo.display, xinfo.screen));
				xinfo.gc = gc1;
				XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x+time*interval, y+time*interval, diameter-time*2*interval, diameter-time*2*interval, 0*64, 360*64);
				time++;			
				xinfo.gc = temp;	
			}
			else {
                        	XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x, y, diameter, diameter, 0*64, 360*64);
                	}
		}
};




void initX(Simon* simon, XInfo& xinfo) {
	xinfo.display = XOpenDisplay("");
	if (!(xinfo.display)) exit (-1);
	xinfo.screen = DefaultScreen(xinfo.display);
	xinfo.width = 800;
	xinfo.height = 400;
	xinfo.window = XCreateSimpleWindow(xinfo.display, DefaultRootWindow(xinfo.display), 
				           10, 10, xinfo.width, xinfo.height, 2,
				           BlackPixel(xinfo.display, xinfo.screen), WhitePixel(xinfo.display, xinfo.screen));
	XSelectInput(xinfo.display, xinfo.window, StructureNotifyMask | ButtonPressMask | KeyPressMask | PointerMotionMask | ButtonMotionMask);
	XMapRaised(xinfo.display, xinfo.window);
	XFlush(xinfo.display);
	usleep(10*1000);
}




bool isInside (int myx, int myy, int cx, int cy) {
	if ((myx-cx)*(myx-cx)+(myy-cy)*(myy-cy) <= 2500) {
		return true;
	}
	else {
		return false;
	}
}






void repaint(XInfo& xinfo);







void handleButtonPress(XInfo &xinfo, XEvent &event, Simon *simon) {
	for (int i = 2; i < simon->getNumButtons()+2; i++) {
		int cx = ((Circle*)d[i])->x+50;
                int cy = ((Circle*)d[i])->y+50;
                if (isInside(event.xbutton.x, event.xbutton.y, cx, cy)) {
			((Circle*)d[i])->fillBlack = true;
			((Text*)d[i+(simon->getNumButtons())])->block = true;
			((Circle*)d[i])->time = 1;
			((Circle*)d[i])->interval = 2;
       			int FPS = 50;
        		for (int j = 0; j < 50; j++) {
                		usleep(1000000 / FPS);
                		repaint(xinfo);
        		}	
			((Circle*)d[i])->fillBlack = false;
                        ((Text*)d[i+(simon->getNumButtons())])->block = false;
			((Circle*)d[i])->time = 0;
			if (simon->getStateAsString() == "HUMAN") {
				if (simon->verifyButton(i-2)) {
					if (simon->getStateAsString() == "WIN") {
						int score = simon->getScore();
                                		stringstream ss;
                                		ss << score;
                                		((Text*)d[0])->s = ss.str();
                                		((Text*)d[1])->s = "You won! Press SPACE to continue";
						needspace = true;
						return;
					}
				}
				else {
					if (simon->getStateAsString() == "LOSE") {
						((Text*)d[1])->s = "You lose. Press SPACE to play again";
						needspace = true;
						return;
					}
				}
			}
		}
	}		
}








void handleMotion(XInfo &xinfo, XEvent &event, Simon *simon) {
	for (int i = 2; i < simon->getNumButtons()+2; i++) {
		int cx = ((Circle*)d[i])->x+50;
		int cy = ((Circle*)d[i])->y+50;
		if (isInside(event.xbutton.x, event.xbutton.y, cx, cy)) {
			((Circle*)d[i])->linewidth = 5;
		}
		else {
			((Circle*)d[i])->linewidth = 1;
		}
	}
}

			







void handleResize(XInfo &xinfo, XEvent &event, Simon *simon) {
	XConfigureEvent xce = event.xconfigure;
	if (xce.width != xinfo.width || xce.height != xinfo.height) {
		xinfo.width = xce.width;
		cout<<"new window x" <<xinfo.width<<endl;
		xinfo.height = xce.height;
	}
	int buttonNum = simon->getNumButtons();
        int space = (xinfo.width-100*buttonNum)/(buttonNum+1);
        for (int i = 2; i < buttonNum+2; i++) {
                        ((Circle *)d[i])->x = space+(100+space)*(i-2);
                        ((Circle *)d[i])->y = xinfo.height/2-50;
        }
        for (int i = buttonNum+2; i < d.size(); i++) {
                ((Text *)d[i])->x = space+(100+space)*(i-2-buttonNum)+45;
                ((Text *)d[i])->y = xinfo.height/2+10;
        }
}





void repaint(XInfo &xinfo) {
	XClearWindow(xinfo.display, xinfo.window);
	for (int i = 0; i < d.size(); i++) {
		d[i]->paint(xinfo);
	}
	XFlush(xinfo.display);
}
	


void handleSpace(XInfo &xinfo, Simon *simon) {
	for (int i = 2; i < 2+simon->getNumButtons(); i++) {
                ((Circle*)d[i])->y = xinfo.height/2-50;
        }
	for (int i = 2+simon->getNumButtons();i < d.size(); i++) {
		((Text*)d[i])->y = xinfo.height/2+10;
	}
	needspace = false;
	repaint(xinfo);
	if (simon->getStateAsString() == "LOSE" || simon->getStateAsString() == "WIN"
		|| simon->getStateAsString() =="START") {
		if (simon->getStateAsString() == "LOSE") ((Text*)d[0])->s = "0";
		((Text*)d[1])->s = "Watch what I do ...";
		simon->newRound();
		while (true) {
			string state = simon->getStateAsString();
			if (state == "COMPUTER") {
				int statenum = simon->nextButton();
				((Circle*)d[statenum+2])->fillBlack = true;
                		((Text*)d[statenum+(simon->getNumButtons())+2])->block = true;
                		((Circle*)d[statenum+2])->time = 1;
				((Circle*)d[statenum+2])->interval = 2;
               			int FPS = 50;
                		for (int j = 0; j < 25; j++) {
					repaint(xinfo);
                			usleep(1000000 / FPS);
                		}
              			((Circle*)d[statenum+2])->fillBlack = false;
                		((Text*)d[statenum+2+(simon->getNumButtons())])->block = false;
                		((Circle*)d[statenum+2])->time = 0;
				((Circle*)d[statenum+2])->interval = 2;
				repaint(xinfo);
				usleep(250*1000);
			}		
			else {((Text*)d[1])->s = "Now it's your turn";break;}
		}
	}
	else {
		cout<<"cannot start a new game"<<endl;
	}
}

			
unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000+tv.tv_usec;
}




void eventloop(XInfo& xinfo, Simon *simon) {
	XEvent event;
	KeySym key;
	char text[10];
	unsigned long lastRepaint = 0;
	int counter = 0;	
	while (true) {
	 	if (XPending(xinfo.display) > 0) {
			XNextEvent(xinfo.display, &event);
			switch (event.type) {
				case ButtonPress:
					handleButtonPress(xinfo, event, simon);
					break;
				case MotionNotify:
					handleMotion(xinfo, event, simon);		   
					break;
				case ConfigureNotify:
                                	handleResize(xinfo, event, simon);
                                	break;
				case KeyPress:
					int i = XLookupString((XKeyEvent*)&event, text, 10, &key, 0);
					if (i == 1 && text[0] == 'q') {
						cout << "Terminated normally."<<endl;
						XCloseDisplay(xinfo.display);
						return;
					}
					else if (i == 1 && text[0] == ' ') { 
						handleSpace(xinfo, simon);		
					}
					else { };
					break;
			}
		}
		usleep(10*1000);
		repaint(xinfo);
		unsigned long end = now();
		if (needspace) {
			if (end-lastRepaint > 100000 / 60) {
				XClearWindow(xinfo.display, xinfo.window);
	        		double scale = simon->getNumButtons() / (2*3.1416);
				double num = 0.05;
                		for (int i = 2; i < simon->getNumButtons()+2; i++) {
                        		((Circle*)d[i])->y += sin((double)counter*num+(double)i*scale);
               	 		}
				for (int i = 2+simon->getNumButtons(); i < d.size(); i++) {
					((Text*)d[i])->y += sin((double)counter*num+(double)(i-simon->getNumButtons())*scale);
				}
                		repaint(xinfo);
				lastRepaint = now();
				counter++;
			}
		}
        }
	
}
					
	
	

int main( int argc, char* argv[]) {
	int n = 4;
	if (argc > 1) n = atoi(argv[1]); 
	Simon* simon = new Simon(n, true);
	XInfo xinfo;
	initX(simon, xinfo);
	
	GC gc = XCreateGC(xinfo.display, xinfo.window, 0, 0);
	XSetForeground(xinfo.display, gc, BlackPixel(xinfo.display, xinfo.screen));
	XSetBackground(xinfo.display, gc, WhitePixel(xinfo.display, xinfo.screen));
	xinfo.gc = gc;

	XFontStruct *font;
	font = XLoadQueryFont(xinfo.display, "12x24");
	XSetFont(xinfo.display, xinfo.gc, font->fid);
	
	//draw string
	Text* text1 = new Text(20, 40, "0", false);
	d.push_back(text1);
	Text* text2 = new Text(20, 70, "Press SPACE to play", false);
	d.push_back(text2);
	
	//draw circle
	int buttonNum = simon->getNumButtons();
        int space = (xinfo.width-100*buttonNum)/(buttonNum+1);
        for (int i = 0; i < buttonNum; i++) {
		Circle* c = new Circle (space+(100+space)*i, xinfo.height/2-50,100,1, false, 0, 1);
		d.push_back(c);
        }

	//draw number
	for (int i = 0; i < buttonNum; i++) {
		stringstream ss;
                ss << i+1;
                string curNum = ss.str();
		Text *t = new Text(space+(100+space)*i+45, xinfo.height/2+10, curNum, false);
		d.push_back(t);
	}
	
	repaint(xinfo);
	
	cout << "press q to exit"<<endl;
	needspace = true;
	eventloop(xinfo, simon);
	
	delete simon;
	for (int i = 0; i < d.size(); i++) {
		delete d[i];
	}
}

