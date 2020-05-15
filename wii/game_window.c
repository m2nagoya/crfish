#include "game_system.h"
#include <SDL/SDL.h>                    // SDLを用いるために必要なヘッダファイル
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_gfxPrimitives.h>	// 描画関係のヘッダファイル
#include <SDL/SDL_mixer.h>              // SDLでサウンドを用いるために必要なヘッダファイルをインクルード
#include <libcwiimote/wiimote.h>	// Wiiリモコンを用いるために必要なヘッダファイル
#include <libcwiimote/wiimote_api.h>	// Wiiリモコンを用いるために必要なヘッダファイル

/* 画像ファイルパス */
static char gMapImgFile[]              = "map.png";
static char *gCharaImgFile[ CT_NUM ]   = {"player.png","enemy.png","enemy.png","enemy.png","enemy.png","enemy.png","bubble.png","beam.png"};
static char gTitleImgFile[]            = "sea.png";

/* フォント関連 */
static char gFontFileE[] = "font.ttf";
static char *gMsgStr[ MSG_NUM ] = {"GAMEOVER", "CONGRATULATE", "RECORD     %d m"}; 
static TTF_Font *gTTF;
static TTF_Font *gTTF_GameOver;

/* サーフェイス */
static SDL_Surface *gMainWindow;
static SDL_Surface *gTheWorld;
static SDL_Surface *gBG;
static SDL_Surface *gMapImage;
static SDL_Surface *gCharaImage[ CT_NUM ];
static SDL_Surface *gMessages[ MSG_NUM ];
static SDL_Surface *gBGMask;
static SDL_Surface *gCharaMask[ CT_NUM ];
static SDL_Surface *gTitleImage;

/* OpenCV関連 */
IplImage *gCvBGMask;
IplImage *gCvCharaMask[ CT_NUM ];
IplImage *gCvAnd;

/* 関数 */
static int DisplayStatus(void);
static int MakeBG(void);

/* メインウインドウの表示，設定 */
int InitWindow(void){
    /* メインのウインドウ(表示画面)の作成 */
    gMainWindow = SDL_SetVideoMode(WD_Width * MAP_ChipSize, WD_Height * MAP_ChipSize, 32, SDL_HWSURFACE|SDL_DOUBLEBUF); 
    if(gMainWindow == NULL) 
        return PrintError( SDL_GetError() );

    /* ゲーム画面(フィールドバッファ)の作成 */
    gTheWorld = SDL_CreateRGBSurface(SDL_HWSURFACE, MAP_Width * MAP_ChipSize, MAP_Height * MAP_ChipSize, 32, 0, 0, 0, 0);
    if(gTheWorld == NULL) 
        return PrintError( SDL_GetError() );

    /* 画像の読み込み */
    gMapImage = IMG_Load( gMapImgFile );
    if(gMapImage == NULL)    // マップ画像の読み込み
        return PrintError("failed to open map image."); // 画像の読み込み判定

    /* タイトル画像の読み込み */
    gTitleImage = IMG_Load( gTitleImgFile );
    if(gTitleImage == NULL)    // マップ画像の読み込み
        return PrintError("failed to open map image."); // 画像の読み込み判定
    
    int i;
    for(i=0; i<CT_NUM; i++){
        gCharaImage[i] = IMG_Load( gCharaImgFile[i] );
        if(gCharaImage[i] == NULL){
            return PrintError("failed to open character image.");
        }
        SDL_Rect r = {0}; // 画像は正方形で，下半分がマスクパターン，横にアニメーションパターンがあると想定
        r.w = r.h = gCharaImage[i]->h / 2;
        r.x = 0; 
        r.y = r.h;

        /* マスク画像作成(SDL画像をOpenCVで使う) */
        gCharaMask[i] = SDL_CreateRGBSurface(SDL_HWSURFACE, r.w, r.h, 32, 0xff0000, 0xff00, 0xff, 0xff000000); 
        SDL_BlitSurface(gCharaImage[i], &r, gCharaMask[i], NULL);
        gCvCharaMask[i] = cvCreateImageHeader(cvSize(r.w, r.h), IPL_DEPTH_8U, gCharaMask[i]->format->BytesPerPixel);
        cvSetData(gCvCharaMask[i], gCharaMask[i]->pixels, gCvCharaMask[i]->widthStep);
    }
    for(i=0; i<gCharaNum; i++){ // 画像サイズ，アニメーションパターン数の設定
        gChara[i].pos.w     = gCharaMask[ gChara[i].type ]->w;
        gChara[i].pos.h     = gCharaMask[ gChara[i].type ]->h;
        gChara[i].anipatnum = gCharaImage[ gChara[i].type ]->w / gChara[i].pos.w;
    }
    gCvAnd = cvCreateImage(cvSize(gCharaMask[gID]->w,gCharaMask[gID]->h), IPL_DEPTH_8U, gCharaMask[gID]->format->BytesPerPixel); //マスク合成用
    
    /* フォントの初期化 */
    if(TTF_Init() < 0) 
        return PrintError( TTF_GetError() );

    /* フォントを開く */
    gTTF = TTF_OpenFont( gFontFileE, 80 );  
    if( gTTF == NULL ) 
        return PrintError( TTF_GetError() );

    /* メッセージ作成 */
    SDL_Color cols[ MSG_NUM ] = { {0,0,0}, {255,255,255} }; 
    for(i=0; i<MSG_NUM; i++){
        gMessages[i] = TTF_RenderText_Blended(gTTF, gMsgStr[i], cols[i]);
        if( gMessages[i] == NULL ) 
            return PrintError( TTF_GetError() );
    }
    TTF_CloseFont(gTTF); //フォントを閉じる

    /* スコア表示用にフォントを開いておく(ゲーム中) */
    gTTF = TTF_OpenFont( gFontFileE, 24 ); 
    if( gTTF == NULL ) 
        return PrintError( TTF_GetError() );
    
    /* スコア表示用にフォントを開いておく(ゲーム終了時) */
    gTTF_GameOver = TTF_OpenFont( gFontFileE, 50 ); 
    if( gTTF == NULL ) 
        return PrintError( TTF_GetError() );

    /* 背景の作成 */
    gBG = SDL_CreateRGBSurface(SDL_HWSURFACE, MAP_Width * MAP_ChipSize, MAP_Height * MAP_ChipSize, 32, 0, 0, 0, 0);  
    if(gBG == NULL) 
        return PrintError( SDL_GetError() );
    gBGMask = SDL_CreateRGBSurface(SDL_HWSURFACE, MAP_Width * MAP_ChipSize, MAP_Height * MAP_ChipSize, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
    if(gBGMask == NULL) 
        return PrintError( SDL_GetError() );
    MakeBG();
    BlitWindow();             // ウインドウへの描画
    SDL_EnableKeyRepeat(0,0); //キーリピート無効を設定
    return 0;
}

/* ウインドウの終了処理 */
void DestroyWindow(void){ 
    int i;
    /* サーフェイス,OpenCV,素材の読み込み */
    for(i=0; i<CT_NUM; i++){ 
        cvReleaseImageHeader(&(gCvCharaMask[i]));
        SDL_FreeSurface(gCharaImage[i]);
        SDL_FreeSurface(gCharaMask[i]);
    }

    /* メッセージの読み込み */
    for(i=0; i<MSG_NUM; i++) 
        SDL_FreeSurface(gMessages[i]);
    SDL_FreeSurface(gMapImage);
    cvReleaseImage(&gCvAnd);
    cvReleaseImageHeader(&gCvBGMask);
    SDL_FreeSurface(gBG);
    SDL_FreeSurface(gBGMask);
    SDL_FreeSurface(gTheWorld);
    SDL_FreeSurface(gMainWindow);
    Mix_HaltMusic();                // BGMの停止
    Mix_FreeMusic(music);           // BGMの解放
    Mix_CloseAudio();               // サウンドの停止
    wiimote_speaker_free(&wiimote); // Wiiリモコンのスピーカを解放
    wiimote_disconnect(&wiimote);   // Wiiリモコンとの接続を解除
    if( gTTF ) 
        TTF_CloseFont( gTTF ); // フォント
    if( gTTF_GameOver ) 
        TTF_CloseFont( gTTF_GameOver ); // フォント
    if( TTF_WasInit() )
        TTF_Quit();
}

int count;
/* メインウインドウに対するイベント処理(Wiiリモコンでの処理) */
int WindowWiiEvent(void){
    int i;
    if (wiimote.keys.home) {                // HOMEボタンが押された時
        wiimote_disconnect(&wiimote);	    // Wiiリモコンとの接続を解除
    }
    if(gStatus.restPlayer == 1){
        count++;
        if(gStatus.start == 0){ // ゲーム開始
            if(wiimote.keys.two){
                gStatus.start = 1;
                if(count <= 96) // 効果音の判定
                    Mix_PlayChannel(1,PatinkoSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
            }
        }
        if(gStatus.start == 1){ // ゲーム起動中
            if(wiimote.keys.right || wiimote.keys.two || (2.0<= wiimote.force.z && wiimote.force.z <= 3.0)){ // ジャンプ
                gStatus.keyjump = 1;
                if(gStatus.keyjump == 1){
                    for(i=0; i<gCharaNum; i++){
                        if(gChara[i].type == CT_Player){
                            gStatus.y_prev = gChara[i].point.y;
                            gChara[i].point.y = gChara[i].point.y -4.478;
                        }
                    }
                    gStatus.keyjump = 2;
                    if(count > 96) // 効果音の判定
                        Mix_PlayChannel(1,JumpSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
                }
            }
        }
    }
    return 1;
}

/* マップの配置 */
int MakeBG(void){
    long long int i, j, ret = 0;                                    // gMaps[Width][Height]
    SDL_Rect src  = { 0, 0, MAP_ChipSize, MAP_ChipSize }; // Map素材(縦48×横48の正方形)
    SDL_Rect dst  = {0};                                  // コピー先
    if(gStatus.restPlayer == 1){
        for(i=0; i<MAP_Width; i++, dst.x+=MAP_ChipSize){
            dst.y = 0;
            for(j=0; j<MAP_Height; j++, dst.y+=MAP_ChipSize){
                src.x = gMaps[j][i] * MAP_ChipSize; // srcはmap素材の選択，gMapはmap.dataの数字
                src.y = 0;
                ret += SDL_BlitSurface(gMapImage, &src, gBG, &dst);
                
                src.y = MAP_ChipSize;               // マスク画像をmap素材の下にしく
                ret += SDL_BlitSurface(gMapImage, &src, gBGMask, &dst);
            }
        }
    }
    gCvBGMask = cvCreateImageHeader(cvSize(gBGMask->w,gBGMask->h), IPL_DEPTH_8U, gBGMask->format->BytesPerPixel); //マスク画像作成
    cvSetData( gCvBGMask, gBGMask->pixels, gCvBGMask->widthStep);
    return ret;
}

/* キャラの消去 */
void ClearChara(void){ 
    int i;
    for(i=0; i<gCharaNum; i++){
        if(gChara[i].stts != CS_Disable){
            SDL_Rect dst = gChara[i].pos;
            SDL_BlitSurface(gBG, &(gChara[i].pos), gTheWorld, &dst);
        }
    }
}

/* キャラの配置(キャラの読み込み) */
void PlaceChara(void){ 
    int i;
    for(i=0; i<gCharaNum; i++){
        if(gChara[i].stts != CS_Disable){
            SDL_Rect src = gChara[i].pos;
            src.x = src.w * gChara[i].anipat;
            src.y = 0;
            SDL_Rect dst = gChara[i].pos;
            SDL_BlitSurface( gCharaImage[ gChara[i].type ], &src, gTheWorld, &dst );
        }
    }
}

/* スコアの表示 */
int DisplayStatus(void){
    int ret  = 0;
    char title[ 64 ];
    SDL_Surface *mesF, *mesB,*mesB_GameOver;
    SDL_Color colF = {255,255,255};
    SDL_Color colB = {0,0,0};
    sprintf(title, gMsgStr[MSG_Score], gStatus.scroll_X / 48);
    mesF = TTF_RenderUTF8_Blended(gTTF, title, colF);
    mesB = TTF_RenderUTF8_Blended(gTTF, title, colB);
    mesB_GameOver = TTF_RenderUTF8_Blended(gTTF_GameOver, title, colB);
    if(gStatus.start == 1){
        if(gStatus.restPlayer != 0){
            if(mesB){
                SDL_Rect src = {0,0, mesB->w,mesB->h}; // 影の描画(1ピクセル右下に黒で書く)
                SDL_Rect dst = {WD_Width * MAP_ChipSize - src.w - 16 + 1, 16 + 1};
                ret += SDL_BlitSurface(mesB, &src, gMainWindow, &dst);
                dst.y -= 2;
                ret += SDL_BlitSurface(mesB, &src, gMainWindow, &dst);
                SDL_FreeSurface(mesB);
            }
            if(mesF){
                SDL_Rect src = {0,0, mesF->w,mesF->h}; // 本体の描画(横にずらして重ねることで太字に)
                SDL_Rect dst = {WD_Width * MAP_ChipSize - src.w - 16, 16};
                ret += SDL_BlitSurface(mesF, &src, gMainWindow, &dst);
                dst.x--;
                ret += SDL_BlitSurface(mesF, &src, gMainWindow, &dst);
                SDL_FreeSurface(mesF);
            }
        }
        if(gStatus.restPlayer == 0){
            if(mesB_GameOver){           
                SDL_Rect src = {0,0, mesB_GameOver->w,mesB_GameOver->h}; // 影の描画(1ピクセル右下に黒で書く)
                SDL_Rect dst = {WD_Width * MAP_ChipSize/2 - 196, WD_Height * MAP_ChipSize/2 + 31};
                ret += SDL_BlitSurface(mesB_GameOver, &src, gMainWindow, &dst);
                dst.y -= 2;
                ret += SDL_BlitSurface(mesB_GameOver, &src, gMainWindow, &dst);
                SDL_FreeSurface(mesB);
            }
        }
    }
    return ret;
}

/* タイトル画面の表示 */
int TitleWindow(void){
    SDL_Rect src = {0,0, WD_Width * MAP_ChipSize, WD_Height * MAP_ChipSize}; // 読み込む画像の大きさ
    int ret = SDL_BlitSurface(gTitleImage, &src, gMainWindow, NULL);
    DisplayStatus();
    ret += SDL_Flip(gMainWindow); // 描画更新
    return ret;
}

/* ウィンドウの更新 */
int BlitWindow(void){ 
    SDL_Rect src = { gStatus.scroll_X, 0, WD_Width * MAP_ChipSize, WD_Height * MAP_ChipSize }; //メインウインドウへの転送
    int ret =  SDL_BlitSurface( gTheWorld, &src, gMainWindow, NULL );
    ret =+ SDL_BlitSurface( gBG, NULL, gTheWorld, NULL);
    DisplayStatus(); // 状態更新
    if(gStatus.restPlayer<=0){ // メッセージ
            src.x = 0;
            src.w = gMessages[ gStatus.message ]->w;
            src.h = gMessages[ gStatus.message ]->h;
            SDL_Rect dst = {(WD_Width * MAP_ChipSize - src.w) / 2 - 30, (WD_Height * MAP_ChipSize - src.h) / 2 - 30};
            ret += SDL_BlitSurface(gMessages[ gStatus.message ], &src, gMainWindow, &dst);
    }
    ret += SDL_Flip(gMainWindow); // 描画更新
    return ret;
}
