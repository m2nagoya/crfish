#include "game_system.h"
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

/* BGMファイルパス */
static char BgmFile[] = "bgm.mp3";

/* 効果音ファイルパス */
static char JumpSEFile[]     = "jump.wav";
static char BubbleSEFile[]   = "bubble.wav";
static char ClearSEFile[]    = "clear.wav";
static char GameOverSEFile[] = "gameover.wav";
static char HitSEFile[]      = "hit.wav";
static char PatinkoSEFile[]  = "patinko.wav";

/* BGM */
Mix_Music *music; // BGM

/* 効果音 */
Mix_Chunk *JumpSE;       // ジャンプの効果音
Mix_Chunk *BubbleSE;     // 泡の効果音
Mix_Chunk *ClearSE;      // クリアの効果音
Mix_Chunk *GameOverSE;   // ゲームオーバーの効果音
Mix_Chunk *HitSE;        // ヒットの効果音
Mix_Chunk *PatinkoSE;    // あの音の効果音

/* 関数 */
static Uint32 Timer1( Uint32 interval, void *pram );
static Uint32 Timer2( Uint32 interval, void *pram );

/* main */
int main(int argc,char *argv[]){
    /* 初期化(SDL) */
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0){
        return PrintError( SDL_GetError() );
    }

    /* 初期化(ゲームシステム) */
    if( InitSystem() < 0 ){
        PrintError("failed to initialize System");
        goto DESTROYSYSTEM;
    }

    /* 初期化(ウインドウ) */
    if( InitWindow() < 0 ){
        PrintError("failed to initialize Windows");
        goto DESTROYALL;
    }

    /* サウンドの初期化 */
    if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        printf("failed to initialize SDL_mixer.\n");
        SDL_Quit();
        exit(-1);
    }

    /* BGMの読み込み */
    music = Mix_LoadMUS( BgmFile );
    if(music == NULL)
        return printf("failed to load music.");
    Mix_VolumeMusic(100); //BGMの音量

    /* 効果音の読み込み */
    JumpSE = Mix_LoadWAV( JumpSEFile ); // ジャンプの効果音
    if(JumpSE == NULL)
        return printf("failed to load JumpSE.");
    BubbleSE = Mix_LoadWAV( BubbleSEFile ); // ジャンプの効果音
    if(BubbleSE == NULL)
        return printf("failed to load BubbleSE.");
    ClearSE = Mix_LoadWAV( ClearSEFile ); // クリアーの効果音
    if(ClearSE == NULL)
        return printf("failed to load ClearSE.");
    GameOverSE = Mix_LoadWAV( GameOverSEFile ); // ゲームオーバーの効果音
    if(GameOverSE == NULL)
        return printf("failed to load GameOverSE.");
    HitSE = Mix_LoadWAV( HitSEFile );  // ヒットの効果音
    if(HitSE == NULL)
        return printf("failed to load HitSE.");
    PatinkoSE = Mix_LoadWAV( PatinkoSEFile );  // ヒットの効果音
    if(PatinkoSE == NULL)
        return printf("failed to load PatinkoSE.");

    /* タイマー */
    SDL_TimerID timer1 = SDL_AddTimer(1, Timer1, NULL);
    SDL_TimerID timer2 = SDL_AddTimer(100, Timer2, NULL);

    Mix_PlayMusic(music, -1);          // BGMの再生（繰り返し再生）

    /* タイトル画面 */
    TitleWindow();


    /* ゲーム画面 */
    while(1){
        if(gStatus.scroll_X >= 14400){         //300mでクリア
            gStatus.message = MSG_Clear;
            gStatus.restPlayer--;
            Mix_PlayChannel(1,ClearSE,0);      // 効果音の再生（割り当てたチャンネルで1回再生）
        }
        WindowWiiEvent();                      // ウィンドウ(SDL)イベント(Wii)
        if(gStatus.start == 1){
            MoveChara();                       // 描画処理
            int i,j;
            for(i=0; i<gCharaNum; i++)
                for(j=i+1; j<gCharaNum; j++)
                    Collision( &gChara[i], &gChara[j] );
            ClearChara();
            PlaceChara();
            BlitWindow();
        }
    }
    SDL_RemoveTimer( timer2 ); //終了処理
    SDL_RemoveTimer( timer1 );
DESTROYALL:
    DestroyWindow();
DESTROYSYSTEM:
    DestroySystem();
    SDL_Quit();
    return 0;
}

/* エラーメッセージ表示 */
int PrintError( const char *str ){
    fprintf( stderr, "%s\n", str );
    return -1;
}

int count;
/* タイマー処理1(スクロールの更新) */
Uint32 Timer1( Uint32 interval, void *pram ){
    if(gStatus.restPlayer == 1){
        if(gStatus.start == 1){
            if(gStatus.bubble == 0){ // 泡をとる前
                gStatus.scroll_X += 1;
            }
            if(gStatus.bubble == 1){ // 泡をとった時
                count++;
                if(count <  400){
                    gStatus.scroll_X += 2;
                }
                if(count == 400){
                    gStatus.bubble = 0;
                    count = 0;
                }
            }
        }
    }
    return interval;
}

/* タイマー処理2(アニメーションの更新) */
Uint32 Timer2( Uint32 interval, void *pram ){
    int i;
    for(i=0; i<gCharaNum; i++){
        gChara[i].anipat = (gChara[i].anipat+1) % gChara[i].anipatnum;
    }
    return interval;
}
