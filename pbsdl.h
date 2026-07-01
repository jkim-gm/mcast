// pbsdl.h
//

#include <windows.h>
#include <iostream>
#include <stdarg.h>

#include <iostream>
#include <list>
#include <vector>
using namespace std;

int pbsdl_load(HWND hParWnd, char* pFile, int iMute);
void pbsdl_close();

// Jitter buffer — call pbsdl_jbuf_config before pbsdl_load for UDP streams
void   pbsdl_jbuf_config(int enable, double target_sec, int max_bytes, double reopen_ratio);
double pbsdl_jbuf_getduration();   // seconds of packet data currently buffered
int    pbsdl_jbuf_getbytes();      // bytes of packet data currently buffered
int    pbsdl_jbuf_isbuffering();   // 1 = pre-buffering (output frozen), 0 = playing
double pbsdl_jbuf_getpercent();    // fill level as % of target (0–100)

void pbsdl_resize(int x, int y, int w, int h, int iRefresh=0);
void pbsdl_refresh();
bool pbsdl_ispaused();
void pbsdl_pause();
void pbsdl_stop();
void pbsdl_play();
void pbsdl_stepfw();
void pbsdl_seekto(double dPos);
void pbsdl_mute(int iMute);
int  pbsdl_isMute();
void pbsdl_jumpTo(int iFrames);
void pbsdl_volume(int vol);

__int64 pbsdl_getduration();
double pbsdl_getposition();
double pbsdl_getptsbyfrmidx(int frmidx);
int pbsdl_getnbframes();
int pbsdl_getcurframe();
double pbsdl_getcurpts();
int pbsdl_getimagerect(SIZE* pSz, RECT* pRt);

typedef void (*pbsdl_rendercallback)(void* pArg, double dTm);
void pbsdl_setrendercallback(pbsdl_rendercallback ptr);
void pbsdl_setcbreceiver(void* pcbrcv);

int sdl2_init(int x, int y, int wx, int wy, HWND hParWnd);
void sld2_loop(int* piLoop);
void sdl2_release();

void sdl2_begin();
void sdl2_end();

class Point {
public:
	Point() {};
	Point(int x, int y) { this->x = x; this->y = y; };
	int x, y;
};

class PolygonShape {
public:
	PolygonShape(std::vector<Point> vertices);
	~PolygonShape();
	Point GetCenter(void);
	Point * GetVertices(void);
	int GetNumberOfVertices(void);
private:
	Point * vertices;
	Point center;
	int length;
};
