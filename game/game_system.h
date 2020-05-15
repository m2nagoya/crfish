#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <opencv/cv.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_gfxPrimitives.h>

/* マップサイズ */
enum {
    MAP_Width    = 340,  //map.dataのチップの数（横）
    MAP_Height   = 13,   //map.dataのチップの数（縦）
    WD_Width     = 20,   //表示するウィンドウの大きさ(横)
    WD_Height    = 13,   //表示するウィンドウの大きさ(縦)
    MAP_ChipSize = 48
};

/* マップの種類 */
typedef enum {
    MT_Wall      = 0,           /* 壁 */
    MT_Floor     = 1,           /* 床 */
    MT_Seanoon   = 2,           /* 海（昼） */
    MT_Seanight  = 3            /* 海（夜） */
} MapType;

/* メッセージ */
enum {
    MSG_GameOver = 0,
    MSG_Clear    = 1,
    MSG_Score    = 2,
    MSG_NUM      = 3            /* メッセージの数 */
};

/* キャラクタータイプ */
typedef enum {
    CT_Player    = 0,           /* プレイヤー(鳥) */
    CT_Enemy1    = 1,           /* 敵1 */
    CT_Enemy2    = 2,           /* 敵2 */
    CT_Enemy3    = 3,           /* 敵3 */
    CT_Enemy4    = 4,           /* 敵4 */
    CT_Enemy5    = 5,           /* 敵5 */
    CT_Bubble    = 6,           /* 泡 */
    CT_Shoot     = 7,           /* ビーム */
    CT_NUM       = 8            /* タイプの数 */
} CharaType;

/* キャラクターの状態 */
typedef enum {
    CS_Enable    = 0,           /* 表示 */
    CS_Disable   = 1,           /* 非表示 */
    CS_NUM       = 2            /* 状態の個数 */
} CharaStts;

/* キャラクターの情報 */
typedef struct {
    CharaType    type;
    SDL_Rect     pos;           /* 位置(ゲーム画面上の座標) */ 
    CvPoint2D32f point;         /* 計算上の座標(基準値は動き出す前のpos) */
    CvPoint      base;          /* 基準位置 */
    float        dist;          /* 移動量 */
    CharaStts    stts;          /* 状態 */
    int          anipat;        /* アニメーションパターン */
    int          anipatnum;     /* パターン数 */
} CharaInfo;

/* ゲームの状態 */
typedef struct {
    int          scroll_X;      // 画面上端のX座標，スクロール用
    int          bubble;        // 泡をとったかの判定   
    int          restPlayer;    // 残人数 
    int          score;         // 得点
    int          message;       // メッセージ
    int          keyjump;       // ジャンプのフラグ
    int          y_prev;        // ジャンプ前のyの座標
    int          y_temp;        // ジャンプの基準点のy座標（保存用）
    int          shoot;         // ビームのフラグ
    int          start;
} GameStts; 

/* 変数 */
extern CharaInfo         *gChara;                        // キャラクター情報
extern int               gCharaNum;                      // キャラクターの数
extern int               gID;                            // プレイヤーの番号
extern GameStts          gStatus;
extern float             gDist[ CT_NUM ];                // キャラ別移動量の基準値
extern MapType           gMaps[ MAP_Height ][ MAP_Width ];
extern IplImage          *gCvBGMask;
extern IplImage          *gCvCharaMask[ CT_NUM ];
extern IplImage          *gCvAnd;

extern Mix_Music         *music;                         // BGM
extern Mix_Chunk         *JumpSE;                        // ジャンプの効果音
extern Mix_Chunk         *BubbleSE;                      // 泡の効果音
extern Mix_Chunk         *ClearSE;                       // クリアの効果音
extern Mix_Chunk         *GameOverSE;                    //ゲームオーバーの効果音
extern Mix_Chunk         *HitSE;                         // ヒットの効果音
extern Mix_Chunk         *PatinkoSE;                     // あの音の効果音

/* 関数 */
/* shoot.c */
extern int  PrintError( const char *str );
/* system.c */
extern int  InitSystem(void);
extern void DestroySystem(void);
extern void MoveChara(void);
extern void Collision( CharaInfo *ci, CharaInfo *cj );
extern void ShootBeam(void);
/* window.c */
extern int  InitWindow(void);
extern void DestroyWindow(void);
extern int  WindowEvent(void);
extern int  WindowWiiEvent(void);
extern void ClearChara(void);
extern void PlaceChara(void);
extern void PlaceCharaHp(void);
extern int  BlitWindow(void);
extern int  TitleWindow(void);

#endif
