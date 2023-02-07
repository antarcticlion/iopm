/*
PC-9801/9821 series I/O Port manipulator

Copyright (C) 2023 antarcticlion

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

*/
//-------------------------------------------------------------------------
/**
* @file iopm.c
* @brief iopm プログラム本体
* @author antarcticlion
* @date 05Feb2023
* @details PC-9801/9821シリーズのI/O Portを自由に読み書きするためのms-dos/freedosプログラムです。
* @details 
* @details 注：ファイルは必ずUTF-8で保存すること。コンパイル時にコンパイラ側でUTF-8からSJISに変換します。
*/
//-------------------------------------------------------------------------

#pragma pack(1)

#include "iopm.h"

//-------------------------------------------------------------------------
/**
* @brief バーチャルカーソルの表示位置を更新する。
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details はみ出したらループする。
*/
void inc_moji_pos(){
	vposx++;
	if(vposx > 79){
		vposx -= 80;
		vposy++;
	}
	if(vposy > 24){
		vposy -= 25;
	}
}


//-------------------------------------------------------------------------
/**
* @brief 指定された文字コードはSJISかどうかを判定する
* @param[in] 判定する1バイトコード
* @param[out] 無し
* @return 1=SJIS 0=SJISじゃない
* @details 渡された8ビットデータが、SJIS漢字文字コードの1バイト目かどうかを判定する。
*/
uint8_t is_SJIS(uint8_t code){
	uint8_t result = 0;
	if(      (code > 0x80) && (code < 0xA0)){
		result = 1;
	}else if((code > 0xDF) && (code < 0xFD)){
		result = 1;
	}
	return result;
}



//-------------------------------------------------------------------------
/**
* @brief バーチャルカーソルの位置に任意の属性で半角1バイト文字を1文字書き込む
* @param[in] 文字データ、属性
* @param[out] 無し
* @return 無し
* @details バーチャルカーソルの位置に任意の属性で半角1バイト文字を1文字書き込む
*/
void vwrite_moji(uint16_t value, uint16_t attr){
	uint16_t __far *addr_code = (uint16_t __far *)0xA0000000;
	uint16_t __far *addr_attr = (uint16_t __far *)0xA0002000;

	addr_code[(vposy * 80) + vposx] = (value & 0x00FF);
	addr_attr[(vposy * 80) + vposx] = (attr | ATTR_VISIBLE);

	inc_moji_pos();
}

//-------------------------------------------------------------------------
/**
* @brief バーチャルカーソルの位置に任意の属性で漢字を1文字書き込む
* @param[in] 文字データ、属性
* @param[out] 無し
* @return 無し
* @details バーチャルカーソルの位置に任意の属性で漢字を1文字書き込む
*/
void vwrite_kanji(uint16_t value, uint16_t attr){
	uint16_t __far *addr_code = (uint16_t __far *)0xA0000000;
	uint16_t __far *addr_attr = (uint16_t __far *)0xA0002000;

	addr_code[(vposy * 80) + vposx]     = (value & 0xFF7F);
	addr_code[(vposy * 80) + vposx + 1] = (value & 0x0080);
	addr_attr[(vposy * 80) + vposx]     = (attr | ATTR_VISIBLE);
	addr_attr[(vposy * 80) + vposx + 1] = (attr | ATTR_VISIBLE);

	inc_moji_pos();
	inc_moji_pos();
}



//-------------------------------------------------------------------------
/**
* @brief メイン画面を表示する
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details メイン画面を表示する
*/
void write_main_frame(){
	vposx = 0;
	vposy = 0;
	for(uint16_t index = 0; index < (80 * 24); index++){
		vwrite_moji(op_frame[index], (ATTR_COLOR_BLUE | ATTR_VISIBLE));
	}
}

//-------------------------------------------------------------------------
/**
* @brief SJIS文字コードをVRAMに格納される形式(JIS)に変換する
* @param[in] SJISの1バイト目、2バイト目
* @param[out] 無し
* @return VRAM形式(JIS)の漢字コード
* @details printf書式ではないことに注意。
*/
uint16_t SJIS_to_VRAM(uint8_t code1, uint8_t code2){
  code1 <<= 1;
  if( code2 < 0x9F ){
    if( code1 < 0x3F ){
		code1 += 0x1F;
	}else{
		code1 -= 0x61;
	}
    if( code2 > 0x7E ){
		code2 -= 0x20;
	}else{
		code2 -= 0x1F;
	}
  }else{
    if( code1 < 0x3F ){
		code1 += 0x20;
	}else{
		code1 -= 0x60;
	}
    code2 -= 0x7E;
  }
  code1 -= 0x20;

  return (((uint16_t)code2)<< 8) + ((uint16_t)code1);
}




//-------------------------------------------------------------------------
/**
* @brief 文字列を任意の位置・属性でVRAMに書き込む
* @param[in] 文字列、属性、Ｘ、Ｙ
* @param[out] 無し
* @return 無し
* @details printf書式ではないことに注意。
*/
void VRAM_print(uint8_t *line, uint8_t attr, uint16_t vx, uint16_t vy){
	uint8_t *curr = line;
	uint8_t code;

	vposx = vx;
	vposy = vy;

	while(code = *curr++){
		(is_SJIS(code) ? vwrite_kanji(SJIS_to_VRAM(code, *curr++), (uint16_t)attr) : vwrite_moji((uint16_t)code, (uint16_t)attr) );
	};
}

//-------------------------------------------------------------------------
/**
* @brief 2桁の数値文字列を任意の位置・属性でVRAMに書き込む
* @param[in] 値、属性、ｘ、ｙ
* @param[out] 無し
* @return 無し
* @details 文字列ではないことに注意
*/
void VRAM_print_byte(uint16_t value, uint8_t attr, uint16_t vx, uint16_t vy){
	vposx = vx;
	vposy = vy;
	vwrite_moji( (value >> 8),   (uint16_t)attr );
	vwrite_moji( (value & 0xFF), (uint16_t)attr );
}



//-------------------------------------------------------------------------
/**
* @brief 8ビットの数値を16ビット2桁16進数文字列に変換する
* @param[in] 値
* @param[out] 無し
* @return 無し
* @details 8ビットの数値を16ビット2桁16進数文字列に変換する
*/
uint16_t byte_str(uint8_t value){
	uint16_t result = nible_digit[value >> 4];
	result <<= 8;
	result |= nible_digit[value & 0x0F];
	return result;
}

//-------------------------------------------------------------------------
/**
* @brief 4桁の数値文字列を任意の位置・属性でVRAMに書き込む
* @param[in] 値、属性、ｘ、ｙ、
* @param[out] 無し
* @return 無し
* @details 文字列ではないことに注意
*/
void VRAM_print_word(uint32_t value, uint8_t attr, uint16_t vx, uint16_t vy){
	vposx = vx;
	vposy = vy;
	vwrite_moji( ((value >> 24) & 0xFF),   (uint16_t)attr );
	vwrite_moji( ((value >> 16) & 0xFF),   (uint16_t)attr );
	vwrite_moji( ((value >> 8) & 0xFF),    (uint16_t)attr );
	vwrite_moji( (value & 0xFF),           (uint16_t)attr );
}

//-------------------------------------------------------------------------
/**
* @brief 16ビットの数値を32ビット4桁16進数文字列に変換する
* @param[in] 値
* @param[out] 無し
* @return 文字列
* @details 16ビットの数値を32ビット4桁16進数文字列に変換する
*/
uint32_t word_str(uint16_t value){
	uint32_t result = nible_digit[(value >> 12)  & 0x0F];
	result <<= 8;
	result |= nible_digit[(value >> 8)  & 0x0F];
	result <<= 8;
	result |= nible_digit[(value >> 4)  & 0x0F];
	result <<= 8;
	result |= nible_digit[value & 0x0F];
	return result;
}

//-------------------------------------------------------------------------
/**
* @brief デバッグ用カウンタ表示
* @param[in] ｘ、ｙ
* @param[out] 無し
* @return 無し
* @details デバッグ用カウンタ表示
*/
void debug_counter_proc(uint16_t vx, uint16_t vy){
	degub_cnt++;
	VRAM_print_word(word_str(degub_cnt), (ATTR_COLOR_SKY | ATTR_REVERSE), vx, vy);
}

//-------------------------------------------------------------------------
/**
* @brief ログを再描画する。
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details 最新ログは黄色でハイライト、次に書き込む位置はブリンク。
*/
void disp_log(){
	for(uint8_t curr_x = 0; curr_x < 2; curr_x++){
		for(uint8_t curr_y = 0; curr_y < 20; curr_y++){
			uint8_t attr = (((lastlog_x == curr_x) && (lastlog_y == curr_y)) ? (ATTR_COLOR_YELLOW | ATTR_REVERSE) : ATTR_COLOR_WHITE );
			attr |= (((log_x == curr_x) && (log_y == curr_y)) ? ATTR_BLINK : 0 );
			if(logs[curr_x][curr_y].avail){
				VRAM_print(" *  :  **  : __00 : __00", attr, (curr_x ? 54 : 27), 2 + curr_y);
				if(logs[curr_x][curr_y].r_w){
					VRAM_print("W", ATTR_COLOR_RED, (curr_x ? 55 : 28), 2 + curr_y);
				}else{
					VRAM_print("R", ATTR_COLOR_SKY, (curr_x ? 55 : 28), 2 + curr_y);
				}
				if(logs[curr_x][curr_y].b_w){
					VRAM_print("16", ATTR_COLOR_WHITE,  (curr_x ? 61 : 34), 2 + curr_y);
				}else{
					VRAM_print(" 8", ATTR_COLOR_YELLOW, (curr_x ? 61 : 34), 2 + curr_y);
				}
					VRAM_print_word(word_str(logs[curr_x][curr_y].addr), ATTR_COLOR_WHITE, (curr_x ? 67 : 40), 2 + curr_y);
				if(logs[curr_x][curr_y].b_w){
					VRAM_print_word(word_str(logs[curr_x][curr_y].data), ATTR_COLOR_WHITE, (curr_x ? 74 : 47), 2 + curr_y);
				}else{
					VRAM_print_byte(byte_str(logs[curr_x][curr_y].data), ATTR_COLOR_WHITE, (curr_x ? 76 : 49), 2 + curr_y);
				}
			
			}else{
				VRAM_print("--- : ---- : ---- : ----", attr, (curr_x ? 54 : 27), 2 + curr_y);
			}
		}
	}
}

//-------------------------------------------------------------------------
/**
* @brief IOポート読み書きをログに残す。
* @param[in] 読み書き、8/16、アドレス、データ
* @param[out] 無し
* @return 無し
* @details 残すのはR/W、8/16、アドレス、データ。
* @details タイムスタンプもあったほうがいい？
*/
void logger(uint8_t r_w, uint8_t b_w, uint16_t addr, uint16_t data){
	logs[log_x][log_y].avail = 1;
	logs[log_x][log_y].r_w = r_w;
	logs[log_x][log_y].b_w = b_w;
	logs[log_x][log_y].addr = addr;
	logs[log_x][log_y].data = data;
	lastlog_x = log_x;
	lastlog_y = log_y;
	if(++log_y > 19){
		log_y = 0;
		++log_x;
		log_x &= 1;
	}
	disp_log();
}

//-------------------------------------------------------------------------
/**
* @brief 16ビット読み込み
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details IOポートから16ビットのデータを読み込む。アドレスとデータは読み込み後にログに残す。
*/
void io_read_16bit(){
	uint16_t value = inpw(addr_digit);
	logger(0,1,addr_digit, value);
}

//-------------------------------------------------------------------------
/**
* @brief 16ビット書き込み
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details IOポートに16ビットのデータを書き込む。アドレスとデータは書き込み前にログに残す。
*/
void io_write_16bit(){
	logger(1,1,addr_digit, word_digit);
	outpw(addr_digit, word_digit);
}

//-------------------------------------------------------------------------
/**
* @brief 8ビット読み込み
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details IOポートから８ビットのデータを読み込む。アドレスとデータは読み込み後にログに残す。
*/
void io_read_8bit(){
	uint8_t value = inp(addr_digit);
	logger(0,0,addr_digit, value);
}

//-------------------------------------------------------------------------
/**
* @brief 8ビット書き込み
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details IOポートに８ビットのデータを書き込む。アドレスとデータは書き込み前にログに残す。
*/
void io_write_8bit(){
	logger(1,0,addr_digit, byte_digit);
	outp(addr_digit, byte_digit);
}

//-------------------------------------------------------------------------
/**
* @brief 数値の再描画
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details アドレスと、書き込み用の数値、およびカーソルを再描画する。
*/
void redraw_digit(){
	VRAM_print_word(word_str(addr_digit), ATTR_COLOR_WHITE, 18, 1);
	VRAM_print_word(word_str(word_digit), ATTR_COLOR_WHITE, 18, 3);
	VRAM_print_byte(byte_str(byte_digit), ATTR_COLOR_WHITE, 20, 5);

	uint16_t __far *addr_attr = (uint16_t __far *)0xA0002000;

	addr_attr[(((cursol_y * 2) + 1) * 80) + (cursol_x + ((cursol_y == 2) ? 20 : 18))] = (ATTR_COLOR_YELLOW | ATTR_REVERSE | ATTR_VISIBLE);
}

//-------------------------------------------------------------------------
/**
* @brief カーソル位置の数値を1つ上げる。
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details カーソル位置の数値を1つ上げる。桁の繰り上がりもする。
*/
void value_up(){
	switch(cursol_y){
	case 0:
		addr_digit += (0x0001 << ((3-cursol_x) * 4));
		break;
	case 1:
		word_digit += (0x0001 << ((3-cursol_x) * 4));
		break;
	case 2:
		byte_digit += (0x01   << ((1-cursol_x) * 4));
		break;
	}
	redraw_digit();
}

//-------------------------------------------------------------------------
/**
* @brief カーソル位置の数値を1つ下げる。
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details カーソル位置の数値を1つ下げる。桁の繰り下がりもする。
*/
void value_down(){
	switch(cursol_y){
	case 0:
		addr_digit -= (0x0001 << ((3-cursol_x) * 4));
		break;
	case 1:
		word_digit -= (0x0001 << ((3-cursol_x) * 4));
		break;
	case 2:
		byte_digit -= (0x01   << ((1-cursol_x) * 4));
		break;
	}
	redraw_digit();
}

//-------------------------------------------------------------------------
/**
* @brief カーソル上移動
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details カーソルを上に移動する。行き過ぎるとループする
*/
void cursol_up(){
	if(--cursol_y > 2){
		cursol_y = 2;
	}
	if(cursol_y == 2){
		cursol_x &= 1;
	}
	redraw_digit();
}

//-------------------------------------------------------------------------
/**
* @brief カーソル左移動
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details カーソルを左に移動する。行き過ぎるとループする
*/
void cursol_left(){
	--cursol_x;
	cursol_x &= 0x03;
	if(cursol_y == 2){
		cursol_x &= 1;
	}
	redraw_digit();
}

//-------------------------------------------------------------------------
/**
* @brief カーソル右移動
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details カーソルを右に移動する。行き過ぎるとループする
*/
void cursol_right(){
	++cursol_x;
	cursol_x &= 0x03;
	if(cursol_y == 2){
		cursol_x &= 1;
	}
	redraw_digit();
}


//-------------------------------------------------------------------------
/**
* @brief カーソル下移動
* @param[in] 無し
* @param[out] 無し
* @return 無し
* @details カーソルを下に移動する。行き過ぎるとループする
*/
void cursol_down(){
	if(++cursol_y > 2){
		cursol_y = 0;
	}
	if(cursol_y == 2){
		cursol_x &= 1;
	}
	redraw_digit();
}

//-------------------------------------------------------------------------
/**
* @brief キーボードセンス＆読み込み
* @param[in] 無し
* @param[out] 無し
* @return uint8_t 有効なキーコード、または０か１
* @details 使用されるキー意外は無視される。
* @details センスされるキーは以下。
* @details 0x00, //ESC
* @details 0x34, //SPC
* @details 0x1C, //ENTER
* @details 0x3A, //UP
* @details 0x3B, //LEFT
* @details 0x3C, //RIGHT
* @details 0x3D, //DOWN
* @details シフトキー
*/
uint8_t kbread(){
	union REGS  regs_param;						//BIOSコールパラメタ
	union REGS  regs_result;					//BIOSコール戻り値

	regs_param.h.ah = 0x01; 					//指定：キーボードバッファーセンシング
	int86( 0x18, &regs_param, &regs_result);	//BIOSコール実行

	if(regs_result.h.bh){ //入力あり

		regs_param.h.ah = 0x02; 					//指定：シフトキー状態のセンシング
		int86( 0x18, &regs_param, &regs_result);	//BIOSコール実行
		uint8_t shift = regs_result.h.al;			//シフトキー状態の保存

		regs_param.h.ah = 0x00; 					//指定：キーボードバッファーリード
		int86( 0x18, &regs_param, &regs_result);	//BIOSコール実行
		uint8_t keydata = regs_result.h.ah;

		switch(keydata){
		case 0x00: //ESC
			keydata |= 0x80;
			break;
		case 0x1C: //RETURN
			if(shift & 0x01) keydata |= 0x80;
			break;
		case 0x34: //SPACE
			if(shift & 0x01) keydata |= 0x80;
			break;
		case 0x3A: //UP
			if(shift & 0x01) keydata |= 0x80;
			break;
		case 0x3B: //LEFT
			break;
		case 0x3C: //RIGHT
			break;
		case 0x3D: //DOWN
			if(shift & 0x01) keydata |= 0x80;
			break;
		default:
			return 0;
		}
		return keydata;
	}else{
		return 1;
	}
}

//-------------------------------------------------------------------------
/**
* @brief メインループ
* @param[in] 無し
* @param[out] 無し
* @return かならず0
* @details 画面を描画し、キー入力を待ち、キーに合わせて、実行内容を選択する。
* @details 
* @details 画面説明
* @details 
* @details 左上のペインのうち、アドレス、データの数字のところをカーソルが動きます。
* @details 読み書きを行った場合、結果が右のペインに 20x2行分ログとして残ります。
* @details ログ表示は循環し、最新の読み書き行がハイライトされます。
*/
int main(int argc, char *argv[]){
	{//フレーム描画
		printf("\f");
		outp(0x0068, 0x04); //テキスト画面 80桁
		outp(0x0068, 0x07); //font 7x13
		outp(0x0068, 0x0A); //漢字アクセス可
		outp(0x0068, 0x0F); //画面表示可
		uint16_t __far *dst  = (uint16_t __far *)0xA0000000;
		uint16_t __far *src  = (uint16_t __far *)op_frame;
		uint16_t __far *attr = (uint16_t __far *)0xA0002000;
		for(uint16_t index=0; index < (80*23); index++){
			*dst++ = *src++;
			*attr++ = (ATTR_COLOR_SKY | ATTR_VISIBLE);
		}

		VRAM_print("アドレス:    [0x0188]",    ATTR_COLOR_WHITE, 2, 1);
		VRAM_print("データ16:    [0x----]",    ATTR_COLOR_WHITE, 2, 3);
		VRAM_print("データ 8:      [0x--]",    ATTR_COLOR_WHITE, 2, 5);
		VRAM_print("R/W : 8/16 : ADDR : DATA", (ATTR_COLOR_GREEN | ATTR_UNDERLINE), 27, 1);
		VRAM_print("R/W : 8/16 : ADDR : DATA", (ATTR_COLOR_GREEN | ATTR_UNDERLINE), 54, 1);
		VRAM_print("　↑　　　　　　　　　　", ATTR_COLOR_WHITE ,1, 7);
		VRAM_print("←　→　　　　　　　MOVE", ATTR_COLOR_WHITE ,1, 8);
		VRAM_print("　↓　　　　　　　　　　", ATTR_COLOR_WHITE ,1, 9);
		VRAM_print("[SHIFT]+[↑]　　数値　UP", ATTR_COLOR_WHITE ,1, 11);
		VRAM_print("[SHIFT]+[↓]　　数値DOWN", ATTR_COLOR_WHITE ,1, 12);
		VRAM_print("[SPACE] 　　 8ビット読込", ATTR_COLOR_WHITE ,1, 13);
		VRAM_print("[SHIFT]+[SPACE] 　　書込", ATTR_COLOR_WHITE ,1, 14);
		VRAM_print("[RETURN]　　16ビット読込", ATTR_COLOR_WHITE ,1, 15);
		VRAM_print("[SHIFT]+[RETURN]　　書込", ATTR_COLOR_WHITE ,1, 16);
		VRAM_print("[ESC] 　　　　　　　終了", ATTR_COLOR_WHITE ,1, 17);
		VRAM_print("I/O Port manipulator",     ATTR_COLOR_YELLOW ,1, 20);
		VRAM_print("  Ver 1.00",               ATTR_COLOR_YELLOW ,15, 21);
	}

	{//メインループ
		union REGS  regs_param;						//BIOSコールパラメタ
		union REGS  regs_result;					//BIOSコール戻り値

		regs_param.h.ah = 0x03; 					//指定：キーボードインタフェースの初期化
		int86( 0x18, &regs_param, &regs_result);	//BIOSコール実行

		disp_log();
		redraw_digit();

		uint8_t alive = 1;
		while(alive){

			uint8_t kbresult;
			switch(kbresult = kbread()){
			case 0x80:	//ESC
				alive = 0;
				break;
			case 0x1C: //RETURN
				io_read_16bit();
				break;
			case 0x9C: //SHIFT + RETURN
				io_write_16bit();
				break;
			case 0x34: //SPACE
				io_read_8bit();
				break;
			case 0xB4: //SHIFT + SPACE
				io_write_8bit();
				break;
			case 0x3A: //UP
				cursol_up();
				break;
			case 0xBA: //SHIFT + UP
				value_up();
				break;
			case 0x3B: //LEFT
				cursol_left();
				break;
			case 0x3C: //RIGHT
				cursol_right();
				break;
			case 0x3D: //DOWN
				cursol_down();
				break;
			case 0xBD: //SHIFT + DOWN
				value_down();
				break;
			default:
				break;
			}
		}
	}

	return 0;

}



