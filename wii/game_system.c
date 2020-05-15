#include "game_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <opencv/highgui.h>
#include <SDL/SDL.h>	                // SDLを用いるために必要なヘッダファイル
#include <SDL/SDL_gfxPrimitives.h>	// 描画関係のヘッダファイル
#include <SDL/SDL_mixer.h>              // SDLでサウンドを用いるために必要なヘッダファイルをインクルード
#include <libcwiimote/wiimote.h>	// Wiiリモコンを用いるために必要なヘッダファイル
#include <libcwiimote/wiimote_api.h>	// Wiiリモコンを用いるために必要なヘッダファイル

/* データファイルパス */
static char gMapDataFile[] = "map.data";

/* マップなど本体 */
CharaInfo *gChara;                                                // キャラクター情報
int        gCharaNum;                                             // キャラクターの数
int        gID;                                                   // プレイヤーの番号
GameStts   gStatus;
MapType    gMaps[ MAP_Height ][ MAP_Width ];                      //map.dataの格納された数字
float      gDist[ CT_NUM ] = {1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 1.0}; // キャラ別移動量の基準値

/* 関数 */
static CvRect GetIntersectRect( SDL_Rect a, SDL_Rect b );           // 2つの矩形が重なる矩形を返す
static CvPoint2D32f FixPoint( CharaInfo *ch, CvPoint2D32f point );  // 障害物との重なりを調べ，補正した移動先を返す
static CvPoint2D32f MoveByPlayer( CharaInfo *ch );
static CvPoint2D32f MoveByBubble( CharaInfo *ch );
static CvPoint2D32f MoveByEnemy( CharaInfo *ch );

/* ゲームシステム初期化 */
int InitSystem(void){
    /* 乱数初期化 */
    srand(time(NULL));          
           
    /* マップ読み込み */
    FILE* fp = fopen( gMapDataFile, "r" );
    if( fp == NULL )
        return PrintError("failed to open map data.");
    long long int i, j;
    for(i=0; i<MAP_Height; i++){
        for(j=0; j<MAP_Width; j++){
            if(fscanf(fp, "%u", &gMaps[i][j]) != 1){
                fclose( fp );
                return PrintError("failed to load map data.");
            }
        }
    }
    fclose( fp );

    /* ゲーム状態初期化 */
    gStatus.scroll_X = (MAP_Height - WD_Height) * MAP_ChipSize; //スクロールする(余白)部分の定義
    gStatus.score = 0;
    gStatus.bubble = 0;
    gStatus.restPlayer = 1;
    gStatus.keyjump = 0;    // ジャンプを行うときのフラグ
    gStatus.y_prev = 0;     // 比較用
    gStatus.y_temp = 0;     // 格納用
    gStatus.start = 0;
    
    /* キャラクター情報初期化 */
    gCharaNum = CT_NUM; 
    gChara = (CharaInfo*)malloc( sizeof(CharaInfo) * gCharaNum );
    if( gChara == NULL ){
        fclose( fp );
        return PrintError("failed to allocate memory.");
    }
    for(i=0; i<gCharaNum; i++){
        gChara[i].type    = i;
        gChara[i].dist    = gDist[ gChara[i].type ];  // 移動量 
        gChara[i].stts    = CS_Disable;
        gChara[i].anipat  = 0;                        // アニメーションパターン
        gChara[i].point.x = gChara[i].point.y = 0.0;  // 計算上の座標
        gChara[i].base.x  = gChara[i].base.y  = 0;    // 移動方向，基準位置
        if(gChara[i].type == CT_Player){
            gID               = i;
            gChara[i].stts    = CS_Enable;
            gChara[i].point.x = 100;
            gChara[i].point.y = MAP_Height / 3 * MAP_ChipSize; // 計算上の座標
        }
    }
    return 0;
}

/* システム終了処理 */
void DestroySystem(void){
  free( gChara );
  return;
}

double accele = 0; //加速度
/* base方向にdist移動した時の座標を実数計算 */
CvPoint2D32f MoveByPlayer( CharaInfo *ch ){ 
    CvPoint2D32f ret = ch->point;   // 計算上の座標
    ret.x = 100 + gStatus.scroll_X * ch->dist;
    if(gStatus.keyjump == 2){ 
        accele = accele + 0.00001;      // 加速度
        gStatus.y_temp = ret.y;         // y_tempに座標を格納(保存)
        ret.y += (ret.y - gStatus.y_prev) + accele;
        gStatus.y_prev = gStatus.y_temp;
        if(gStatus.keyjump == 1 && accele != 0){
            gStatus.keyjump = 0;        // 初期化
            accele = 0;
        }
    }
    if(gStatus.keyjump == 1){
        gStatus.keyjump = 0;            // 初期化
        accele = 0;
    }
    return ret;
}

/* base方向にdist移動した時の座標を実数計算(泡) */
CvPoint2D32f MoveByBubble( CharaInfo *ch ){
  CvPoint2D32f ret = ch->point;     // 計算上の座標
  float theta  = atan2f( ch->base.x, ch->base.y );
  ret.x += ch->dist * cosf( theta );
  ret.y -= ch->dist * sinf( theta );
  return ret;
}

/* base方向にdist移動した時の座標を実数計算(敵) */
CvPoint2D32f MoveByEnemy( CharaInfo *ch ){
  CvPoint2D32f ret = ch->point;     // 計算上の座標
  float theta  = atan2f( ch->base.x, ch->base.y );
  ret.x += ch->dist * cosf( theta );
  ret.y -= ch->dist * sinf( theta );
  return ret;
}

/* 障害物との重なりを調べ，補正した移動先を返す */
CvPoint2D32f FixPoint( CharaInfo *ch, CvPoint2D32f point ){
  CvRect cr = {point.x, point.y, ch->pos.w, ch->pos.h};

  /* 横方向はプレイヤーのみ画面内に，他は消す */
  int mw = (ch->type == CT_Player) ? 0 : cr.width;
  if((cr.x + mw) <= gStatus.scroll_X){                               // 左側に飛び出した場合
    point.x = gStatus.scroll_X - mw;
    if(ch->type != CT_Player){
        ch->stts = CS_Disable;
        return point;
    }
  }
  mw = (ch->type == CT_Player) ? cr.width : 0;
  if((cr.x + mw) >= (gStatus.scroll_X + (WD_Width * MAP_ChipSize))){ // 右側に飛び出した場合
    point.x = gStatus.scroll_X + (WD_Width * MAP_ChipSize) - mw;
    if(ch->type != CT_Player){
        ch->stts = CS_Disable;
        return point;
    }
  }
  
  /* 縦方向はプレイヤーのみ画面内に，他は消す */
  if(cr.y < 0){                                                      // 下側に飛び出した場合
      point.y = 0;
      if(ch->type != CT_Player){
          ch->stts = CS_Disable;
          return point;
      }
  }
  if((cr.y + cr.height) >= (WD_Height * MAP_ChipSize)){              // 上側に飛び出した場合
      point.y = (WD_Height * MAP_ChipSize) - cr.height;
      if(ch->type != CT_Player){
          ch->stts = CS_Disable;
          return point;
      }
  }

  /* 合成範囲設定 */
  cvSetImageROI(gCvBGMask, cr);  
  cr = cvGetImageROI(gCvBGMask);
  cr.x = cr.y = 0;
  cvSetImageROI(gCvCharaMask[ch->type], cr);
  cvSetImageCOI(gCvAnd, 0);
  cvSetZero(gCvAnd);
  cvSetImageROI(gCvAnd, cr);
  cvAnd(gCvBGMask, gCvCharaMask[ch->type], gCvAnd, NULL); // 画像のANDを取る．色が残っていると重なっていることになる
  
  /* 壁に当たった(青の重なりを調べる) */
  cvSetImageCOI(gCvAnd, 1); 
  if( cvCountNonZero(gCvAnd) ){
    switch(ch->type){
    case CT_Player:      // プレイヤーは戻す
        Mix_PlayChannel(1,HitSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
        point = ch->point;
        gStatus.message = MSG_GameOver;
        gStatus.restPlayer--;
        break;
    default: break;
    }
  }
  
  /* 地面の重なりを調べる(赤の重なりを調べる) */
  cvSetImageCOI(gCvAnd, 3);
  if( cvCountNonZero(gCvAnd) ){
      switch(ch->type){
      case CT_Player:    // プレイヤーは戻す
          Mix_PlayChannel(1,HitSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
          point = ch->point;
          gStatus.message = MSG_GameOver;
          gStatus.restPlayer--;
          break;
      default: break;
      }
  }
  return point;
}

/* キャラの移動 */
void MoveChara(void){
  if(gStatus.restPlayer == 1){
      int i;
      for(i=0; i<gCharaNum; i++){
        if( gChara[i].stts == CS_Disable ){
            switch(gChara[i].type){
            case CT_Enemy1:
                if(gStatus.scroll_X >2400){ // 敵1を出現させる(50m)
                    gChara[i].stts = CS_Enable;
                    gChara[i].base.x = rand() % (MAP_Width * MAP_ChipSize) + 200;
                    gChara[i].base.y = WD_Height * MAP_ChipSize - 200;
                    gChara[i].point.x = gChara[i].base.x;
                    gChara[i].point.y = gChara[i].base.y;
                    gChara[i].dist = gDist[ CT_Enemy1 ];
                }
                continue;
            case CT_Enemy2:
                if(gStatus.scroll_X > 7200){     // 敵2を出現させる(150m)
                    gChara[i].stts = CS_Enable;
                    gChara[i].base.x = rand() % (MAP_Width * MAP_ChipSize) + 200;
                    gChara[i].base.y = WD_Height * MAP_ChipSize - 200;
                    gChara[i].point.x = gChara[i].base.x;
                    gChara[i].point.y = gChara[i].base.y;
                    gChara[i].dist = gDist[ CT_Enemy2 ];
                }
                continue;
            case CT_Enemy3:
                if(gStatus.scroll_X > 7200){     // 敵3を出現させる(150m)
                    gChara[i].stts = CS_Enable;
                    gChara[i].base.x = rand() % (MAP_Width * MAP_ChipSize) + 200;
                    gChara[i].base.y = WD_Height * MAP_ChipSize - 200;
                    gChara[i].point.x = gChara[i].base.x;
                    gChara[i].point.y = gChara[i].base.y;
                    gChara[i].dist = gDist[ CT_Enemy3 ];
                }
                continue;
            case CT_Enemy4:
                if(gStatus.scroll_X > 12000){    // 敵4を出現させる(250m)
                    gChara[i].stts = CS_Enable;
                    gChara[i].base.x = rand() % (MAP_Width * MAP_ChipSize) + 200;
                    gChara[i].base.y = WD_Height * MAP_ChipSize - 200;
                    gChara[i].point.x = gChara[i].base.x;
                    gChara[i].point.y = gChara[i].base.y;
                    gChara[i].dist = gDist[ CT_Enemy4 ];
                }
                continue;
            case CT_Enemy5:
                if(gStatus.scroll_X > 12000){    // 敵5を出現させる(250m)
                    gChara[i].stts = CS_Enable;
                    gChara[i].base.x = rand() % (MAP_Width * MAP_ChipSize) + 200;
                    gChara[i].base.y = WD_Height * MAP_ChipSize - 200;
                    gChara[i].point.x = gChara[i].base.x;
                    gChara[i].point.y = gChara[i].base.y;
                    gChara[i].dist = gDist[ CT_Enemy5 ];
                }
                continue;
            case CT_Bubble:  // 泡を出現させる(100mと200m)
                if((gStatus.scroll_X > 4800 && gStatus.scroll_X < 5000) || (gStatus.scroll_X > 9600 && gStatus.scroll_X <9800)){
                    gChara[i].stts = CS_Enable;
                    gChara[i].base.x = rand() % (MAP_Width * MAP_ChipSize) + 100;
                    gChara[i].base.y = rand() % (MAP_Height * MAP_ChipSize - 200);
                    gChara[i].point.x = gChara[i].base.x;
                    gChara[i].point.y = gChara[i].base.y;
                    gChara[i].dist = gDist[ CT_Bubble ];
                }
                continue;
            default: 
                continue;
            }
        }    
        CvPoint2D32f newpoint;
        /* キャラ別に移動先を計算 */
        switch( gChara[i].stts ){ 
        case CS_Enable:
            switch( gChara[i].type ){
            case CT_Player:
                newpoint = MoveByPlayer( &gChara[i] ); 
                break;
            case CT_Enemy1:
            case CT_Enemy2:
            case CT_Enemy3:
            case CT_Enemy4:
            case CT_Enemy5:
                newpoint = MoveByEnemy( &gChara[i] );
                break;
            case CT_Bubble:
                newpoint = MoveByBubble( &gChara[i] );
                break;
            default: 
                break;
            }
        default: break;
        }
        gChara[i].point = FixPoint( &gChara[i], newpoint ); // 障害物との重なりを調べ，移動先を補正する
        gChara[i].pos.x = gChara[i].point.x;                // ゲーム画面上の座標に反映
        gChara[i].pos.y = gChara[i].point.y;
        gChara[i].pos.x = newpoint.x;
        gChara[i].pos.y = newpoint.y;
        if(gChara[i].type == CT_Player && gChara[i].pos.x >= (gStatus.scroll_X + WD_Width * MAP_ChipSize)){ // プレイヤーははみ出ると終わり
            gStatus.restPlayer--;
        }
      }
  }
}

/* 当たり判定 */
void Collision( CharaInfo *ci, CharaInfo *cj ){ 
    /* 判定が不要な組み合わせを除外 */
    if(ci->stts==CS_Disable || cj->stts==CS_Disable) 
        return;
    CvRect r = GetIntersectRect( ci->pos, cj->pos ); // まずは矩形での重なりを検出
    
    /* 重なった領域のマスク画像を合成 */
    if( r.width>0 && r.height>0 ){
        r.x -= ci->pos.x; 
        r.y -= ci->pos.y;
        cvSetImageROI(gCvCharaMask[ ci->type ], r);
        r.x += ci->pos.x - cj->pos.x;
        r.y += ci->pos.y - cj->pos.y;
        cvSetImageROI(gCvCharaMask[ cj->type ], r);
        r.x = r.y = 0;
        cvSetImageCOI(gCvAnd, 0);
        cvSetImageROI(gCvAnd, r);
        cvAnd(gCvCharaMask[ ci->type ], gCvCharaMask[ cj->type ], gCvAnd, NULL);

        /* プレイヤーに当たった(緑の重なりを調べる) */
        cvSetImageCOI(gCvAnd, 2);
        if( cvCountNonZero(gCvAnd) ){
            if(cj->type == CT_Bubble){                          // 泡の当たり判定
                cj->stts = CS_Disable;
                Mix_PlayChannel(1,BubbleSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
                gStatus.bubble = 1;
            }
            if(ci->type == CT_Player && cj->type == CT_Enemy1){ // 敵1の当たり判定
                cj->stts = CS_Disable;
                gStatus.message = MSG_GameOver;
                gStatus.restPlayer--;
                Mix_PlayChannel(1,HitSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
            }
            if(ci->type == CT_Player && cj->type == CT_Enemy2){ // 敵2の当たり判定
                cj->stts = CS_Disable;
                gStatus.message = MSG_GameOver;
                gStatus.restPlayer--;
                Mix_PlayChannel(1,HitSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
            }
            if(ci->type == CT_Player && cj->type == CT_Enemy3){ // 敵3の当たり判定
                cj->stts = CS_Disable;
                gStatus.message = MSG_GameOver;
                gStatus.restPlayer--;
                Mix_PlayChannel(1,HitSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
            }
            if(ci->type == CT_Player && cj->type == CT_Enemy4){ // 敵4の当たり判定
                cj->stts = CS_Disable;
                gStatus.message = MSG_GameOver;
                gStatus.restPlayer--;
                Mix_PlayChannel(1,HitSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
            }
            if(ci->type == CT_Player && cj->type == CT_Enemy5){ // 敵5の当たり判定
                cj->stts = CS_Disable;
                gStatus.message = MSG_GameOver;
                gStatus.restPlayer--;
                Mix_PlayChannel(1,HitSE,0); // 効果音の再生（割り当てたチャンネルで1回再生）
            }
        }
    }
}

/* 2つの矩形が重なる矩形を返す */
CvRect GetIntersectRect( SDL_Rect a, SDL_Rect b ){ 
  CvRect ret;
  ret.x = (a.x > b.x)? a.x:b.x;
  ret.y = (a.y > b.y)? a.y:b.y;
  ret.width = (a.x + a.w < b.x + b.w) ? a.x + a.w - ret.x : b.x + b.w - ret.x;
  ret.height = (a.y + a.h < b.y + b.h) ? a.y + a.h - ret.y : b.y + b.h - ret.y;
  return ret;
}
