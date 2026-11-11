/* game.h - which iOS game this loader runs (force-included via bqlog.h). Everything game-specific in the loader core keys off these. */
#ifndef GAME_H
#define GAME_H
#define GAME_TAG      "bb3"
#define GAME_DIR      "ux0:data/bb3/"        /* binary + bundle resources live here */
#define GAME_DIR_NS   "ux0:data/bb3"
#define GAME_BIN      GAME_DIR "BBD3"        /* the Mach-O from Payload/BBD3.app */
#define GAME_ARGV0    "BBD3"
#define GAME_MAIN_VM  0x2208u                /* main(): crt start (LC_UNIXTHREAD pc 0x21dc) falls through to it */
#define GAME_MAIN_THUMB 0                    /* ARM code */
/* MainWindow.nib: BBD3AppDelegate (outlets window, glView), UIWindow, EAGLView. No view controller. */
#define GAME_DELEGATE "BBD3AppDelegate"
#define GAME_VIEW     "EAGLView"
#define GAME_VC       NULL
#define GAME_BUNDLE_ID "com.gameloft.BBD3"
/* screen as the game sees it: iPhone 4 portrait, 320x480 points at @2x */
#define GAME_PT_W     320.f
#define GAME_PT_H     480.f
#define GAME_SCALE    2.f
#endif
