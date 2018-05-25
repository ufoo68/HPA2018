/*
ラダー、エレベータ制御プログラム 2018
最終修正日 2018 5/25
*/

#include <Servo.h>

//ラダーキーピン宣言
#define rpn_left 6
#define rpn_rst 7
#define rpn_right 8
//エレベータキーピン宣言
#define epn_up 19
#define epn_rst 18
#define epn_dn 17
//サーボピン宣言
#define rpn_srv 9
#define epn_srv 10
//アナログ入力ピン宣言
#define rpn_ang 0
#define epn_ang 1

//以下は各システム処理のための変数となります。スケッチの書き換えは基本、パラメータの部分のみで行ってください。

//パラメータ
//ラダー初期値
const short r_ang_max = 626;   //アナログ上限値
const short r_ang_min = 328;   //アナログ下限値
const short r_ang_mdl = 470;   //アナログ中央値
const short  r_ang_crange = 8;  //アナログ中央値収束範囲
const boolean r_rvrs = true;     //パルス出力反転フラグ
//エレベータ初期値
const short e_ang_max = 607;   //アナログ上限値
const short e_ang_min = 332;   //アナログ下限値
const short e_ang_mdl = 482;   //アナログ中央値
const short  e_ang_crange = 8;  //アナログ中央値収束範囲
const boolean e_rvrs = false;     //パルス出力反転フラグ
//ラダー、エレベータ共通初期値
const short pulse_max = 2050;     //パルス変換上限値
const short pulse_min = 950;      //パルス変換下限値
const short angle_max = 2000;     //パルス出力上限値
const short angle_min = 1000;     //パルス出力下限値
const char trm_max = 6;           //トリム最大段数
const short mv = 40;              //トリム動作量
const short midle = 1500;         //サーボニュートラル位置
//このシステムの質に関わるパラメータ(あまり大きな値にしないで...)
const short d_avg = 10;           //A/D変換値の平均をとる範囲 1 < d_avg
const short dly = 100;           //トリム受付間隔(ms) チャタリングの防止
const short dband = 0;            //俯瞰幅 < r_mv, e_mv
//デバッグ用フラグ
const boolean v_test = false; //サーボへの信号の電圧のドロップを確認する用

//変数(書き換え禁止)
//トリム用変数
const boolean On = LOW;           //ボタンの状態(on)
const boolean Off = HIGH;         //ボタンの状態(off)
//ラダー用変数
char r_trm_cnt = 0;            //トリム段数
short r_angle;                 //サーボ出力角度
short bfr_angle;               //
short afr_angle;               //
boolean r_left, r_rst, r_right;     //キー入力状態
boolean rk_up, rk_rst, rk_dn;  //キー入力フラグ
short r_value;                 //アナログ読み取り値
short bfr_value[d_avg];               //アナログ読み取り値 before
short r_mv = 0;                //トリム動作量
Servo r_srv;                   //ラダーサーボ変数
unsigned long r_value_sum = 0; //
//エレベータ用変数
char e_trm_cnt = 0;            //トリム段数
short e_angle;                 //サーボ出力角度
short bfe_angle;               //
short afe_angle;               //
boolean e_up, e_rst, e_dn;     //キー入力状態
boolean ek_up, ek_rst, ek_dn;  //キー入力フラグ
short e_value;                 //アナログ読み取り値
short bfe_value[d_avg];               //アナログ読み取り値 before
short e_mv = 0;                //トリム動作量合計
Servo e_srv;                   //エレベータサーボ変数
unsigned long e_value_sum = 0; //
 
//以下よりメイン動作に関するスケッチとなります。書き換えは動作全体への影響を考慮した上で行ってください。

//パルス反転用関数
short reverse(short angle) {
  angle = angle_max - angle + angle_min;
  return angle;
}

//セットアップ
void setup() {
  
//ピン設定と入力プルアップ
  pinMode(rpn_left, INPUT_PULLUP);
  pinMode(rpn_rst, INPUT_PULLUP);
  pinMode(rpn_right, INPUT_PULLUP);
  pinMode(epn_up, INPUT_PULLUP);
  pinMode(epn_rst, INPUT_PULLUP);
  pinMode(epn_dn, INPUT_PULLUP);
  pinMode(rpn_srv, INPUT_PULLUP);
  pinMode(epn_srv, INPUT_PULLUP);
//サーボ初期設定
  r_srv.attach(rpn_srv);
  e_srv.attach(epn_srv);
  r_srv.write(midle);
  e_srv.write(midle);
  bfr_angle = midle;
  bfe_angle = midle;
//A/D変換値初期設定
  for(int i; i<d_avg; i++) {
    bfr_value[i] = r_ang_mdl;
  }
  for(int i; i<d_avg; i++) {
    bfe_value[i] = e_ang_mdl;
  }
  r_value = r_ang_mdl;
  e_value = e_ang_mdl;
}

//メインループ
void loop() {
  
//ラダーアングル処理
  //入力受付
  r_left = digitalRead(rpn_left);
  r_rst = digitalRead(rpn_rst);
  r_right = digitalRead(rpn_right);
  
  //移動平均によるA/D変換地の読み取り
  bfr_value[d_avg-1] = analogRead(rpn_ang);
  for(int i; i<d_avg-1; i++) {
    bfr_value[i] = bfr_value[i+1];
  }
  for(int i; i<d_avg; i++) {
    r_value_sum += bfr_value[i];
  }
  r_value = r_value_sum / d_avg;
  r_value_sum = 0;
  //
  if(v_test) r_value = r_ang_mdl;
  
  //入力ボリュームの出力パルス変換
  if (r_value < r_ang_mdl - r_ang_crange || r_value > r_ang_mdl + r_ang_crange) {
    afr_angle = map(r_value, r_ang_min , r_ang_max , pulse_min + r_mv , pulse_max + r_mv);
    //出力パルスのふかん幅調整
    if (afr_angle <= bfr_angle + dband && afr_angle >= bfr_angle - dband) r_angle = bfr_angle;
    else {
      r_angle = afr_angle;
      bfr_angle = afr_angle;
    }
  }
  //中央(遊び)
  else if (r_value >= r_ang_mdl - r_ang_crange && r_value <= r_ang_mdl + r_ang_crange) { 
    r_angle = map(r_value, r_ang_mdl - r_ang_crange, r_ang_mdl + r_ang_crange, midle + r_mv, midle + r_mv);
  }

//エレベータアングル処理
  //入力受付
  e_up = digitalRead(epn_up);
  e_rst = digitalRead(epn_rst);
  e_dn = digitalRead(epn_dn);
  //移動平均によるA/D変換地の読み取り
  bfe_value[d_avg-1] = analogRead(epn_ang);
  for(int i; i<d_avg-1; i++) {
    bfe_value[i] = bfe_value[i+1];
  }
  for(int i; i<d_avg; i++) {
    e_value_sum += bfe_value[i];
  }
  e_value = e_value_sum / d_avg;
  e_value_sum = 0;
  //
  if(v_test) e_value = e_ang_mdl;
  
  //入力ボリュームの出力パルス変換
  if (e_value < e_ang_mdl - e_ang_crange || e_value > e_ang_mdl + e_ang_crange) {
    afe_angle = map(e_value, e_ang_min , e_ang_max , pulse_min + e_mv , pulse_max + e_mv);
    //出力パルスのふかん幅調整
    if (afe_angle <= bfe_angle + dband && afe_angle >= bfe_angle - dband) e_angle = bfe_angle;
    else {
      e_angle = afe_angle;
      bfe_angle = afe_angle;
    }
  }
  //中央(遊び)
  else if (e_value >= e_ang_mdl - e_ang_crange && e_value <= e_ang_mdl + e_ang_crange) { 
    e_angle = map(e_value, e_ang_mdl - e_ang_crange, e_ang_mdl + e_ang_crange, midle + e_mv, midle + e_mv);
  }
 
//ラダートリム処理
  //キー入力なし
  if (r_left==Off && r_rst==Off && r_right==Off) {
    rk_up = rk_rst = rk_dn = false;
  }
  //UP入力あり
  else if (r_left==On && r_rst==Off && r_right==Off && r_trm_cnt<trm_max && rk_up == false) { 
    r_mv += mv ;
    r_trm_cnt++;
    rk_up = true;
    delay(dly);
  }
  //DOWN入力あり
  else if (r_left==Off && r_rst==Off && r_right==On && r_trm_cnt>-trm_max && rk_dn == false) { 
    r_mv -=  mv;
    r_trm_cnt--;
    rk_dn = true;
    delay(dly);
  }
  //RESET入力あり
  else if (r_left==Off && r_rst==On && r_right==Off && rk_rst == false) { 
    r_mv = 0;
    r_trm_cnt=0; //トリムキーリセット
    rk_rst = true;
    delay(dly);
  }

//エレベータトリム処理
  //キー入力なし
  if (e_up==Off && e_rst==Off && e_dn==Off) {
   ek_up = ek_rst = ek_dn = false;
  }
  //UP入力あり
  else if (e_up==On && e_rst==Off && e_dn==Off && e_trm_cnt<trm_max && ek_up == false) {
    e_mv += mv ;
    e_trm_cnt++;
    ek_up = true;
    delay(dly);
  }
  //DOWN入力あり
  else if (e_up==Off && e_rst==Off && e_dn==On && e_trm_cnt>-trm_max && ek_dn == false) { 
    e_mv -=  mv;
    e_trm_cnt--;
    ek_dn = true;
    delay(dly);
  }
  //RESET入力あり
  else if (e_up==Off && e_rst==On && e_dn==Off && ek_rst == false) { 
    e_mv = 0;
    e_trm_cnt=0; //トリムキーリセット
    ek_rst = true;
    delay(dly);
  }
  
//２軸サーボ動作の範囲の制限
  r_angle = constrain(r_angle, angle_min, angle_max);
  e_angle = constrain(e_angle, angle_min, angle_max);
  
//パルス出力反転(フラグ立ち上がり時のみ)
  if(r_rvrs) r_angle = reverse(r_angle);
  if(e_rvrs) e_angle = reverse(e_angle);
  
//２軸サーボ動作 
  r_srv.write(r_angle);
  e_srv.write(e_angle);
}
