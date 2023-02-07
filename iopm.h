/*
PC-9801/9821 series I/O Port manipulator

Copyright (C) 2023 antarcticlion

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

*/
/**
* @file iopm.h
* @brief iopm ヘッダファイル
* @author antarcticlion
* @date 05Feb2023
* @details PC-9801/9821シリーズのI/O Portを自由に読み書きするためのms-dos/freedosプログラムです。
* @details 
* @details 注：ファイルは必ずUTF-8で保存すること。コンパイル時にコンパイラ側でUTF-8からSJISに変換します。
* @details makeを使わずコンパイルはする場合はこう→　ia16-elf-gcc -march=i8086 -mtune=i8086 -mcmodel=small -fexec-charset=CP932 -o iopm.exe iopm.c
*/

#define MY_NAME "iopm.exe"

#include <stdio.h>
#include <conio.h>
#include <i86.h>
#include <dos.h>

/// VRAM属性　色：黒
#define ATTR_COLOR_BLACK   0x00
/// VRAM属性　色：青
#define ATTR_COLOR_BLUE    0x20
/// VRAM属性　色：赤
#define ATTR_COLOR_RED     0x40
/// VRAM属性　色：マゼンタ
#define ATTR_COLOR_MAGENTA 0x60
/// VRAM属性　色：緑
#define ATTR_COLOR_GREEN   0x80
/// VRAM属性　色：空色
#define ATTR_COLOR_SKY     0xA0
/// VRAM属性　色：黄
#define ATTR_COLOR_YELLOW  0xC0
/// VRAM属性　色：白
#define ATTR_COLOR_WHITE   0xE0
/// VRAM属性　簡易グラフィック
#define ATTR_GRAPH         0x10
/// VRAM属性　アンダーライン
#define ATTR_UNDERLINE     0x08
/// VRAM属性　リバース
#define ATTR_REVERSE       0x04
/// VRAM属性　ブリンク
#define ATTR_BLINK         0x02
/// VRAM属性　表示
#define ATTR_VISIBLE       0x01

 ///数値表示用の文字要素
uint8_t nible_digit[] = "0123456789ABCDEF";

///デバッグ用カウンタ
uint16_t degub_cnt  = 0;
///数値操作用のアドレス値　初期値はFM音源を指す0x0188
uint16_t addr_digit = 0x0188;
///数値書込用の16ビット値　初期値は0x55AA
uint16_t word_digit = 0x55AA;
///数値書込用の 8ビット値　初期値は0xA5
uint8_t  byte_digit = 0xA5;

///ログの1セル分
typedef struct type_logcell {
	/// 0=無効 2=有効
	uint8_t avail;
	/// 0=Read 1=Write
	uint8_t r_w;
	/// 0=8bit 1=16bit
	uint8_t b_w;
	/// 位置合わせ
	uint8_t padding;
	/// アドレス
	uint16_t addr;
	/// データ
	uint16_t data;
} st_logcell;

///次に書き込むログ x
static uint8_t log_x = 0;
///次に書き込むログ y
static uint8_t log_y = 0;
///最終ログ x
static uint8_t lastlog_x = 0;
///最終ログ y
static uint8_t lastlog_y = 0;
///ログ格納用領域
static st_logcell logs[2][20] = {};

///数値操作用カーソル　x
static uint8_t cursol_x = 0;
///数値操作用カーソル　y
static uint8_t cursol_y = 0;

///バーチャルカーソル　x
volatile uint16_t vposx = 0;
///バーチャルカーソル　y
volatile uint16_t vposy = 0;

///メイン画面
static uint16_t op_frame[] = {
	0x009C,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	 0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	  0x0095,0x0095,0x0095,0x0095,0x0095,0x0091,0x0095,0x0095,0x0095,0x0095,
	   0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	    0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	     0x0095,0x0095,0x0091,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	      0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	       0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x009D,

	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,

	0x0093,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	 0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	  0x0095,0x0095,0x0095,0x0095,0x0095,0x0092,0x0000,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,

	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	       	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	       	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	       	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
/*
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
*/
	0x0093,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	 0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	  0x0095,0x0095,0x0095,0x0095,0x0095,0x0092,0x0000,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	 0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	  0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,
	   0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	    0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	     0x0020,0x0020,0x0096,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	      0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,
	       0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0020,0x0096,
	0x009E,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	 0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	  0x0095,0x0095,0x0095,0x0095,0x0095,0x0090,0x0095,0x0095,0x0095,0x0095,
	   0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	    0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	     0x0095,0x0095,0x0090,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	      0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,
	       0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x0095,0x009F,
};
