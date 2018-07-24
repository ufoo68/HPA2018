# 操舵関係
## ハードウェア構成
- Arduino nano
- LANコネクタモジュール
- タクトスイッチ
- サーボモータ
- ジョイスティック
## ケーブルの割り当て
### 略語
|略称  |名前  |ピン番号  |  
|---|---|---|
|e  |エレベータ  |10   |
|r  |ラダー  |9  |  
### LANケーブル left
|線の色  |役割  |ピン番号  |  
|---|---|---|
|橙白 |r left  |6   |
|橙 |neutral  |7  |
|緑白 |r right  |8  |
|青 |e down  |19(A5)  |
|青白 |e neutral  |18(A4)  |
|緑 |e up  |17(A3)  |
|茶白 |none  | none |
|茶 |GND  |GND  |

### LANケーブル right
|線の色  |役割  |ピン番号  |  
|---|---|---|
|橙白 |e servo  |15(A1)   |
|橙 |r servo  |14(A0)  |
|緑白 |none  |none  |
|青 |none  |none  |
|青白 |Vcc  |5v  |
|緑 |Vcc  |5v  |
|茶白 |GND  |GND  |
|茶 |GND  |GND  |

### 信号線
|線の色  |役割  |  
|---|---|
|黄  |r  |  
|緑  |e  |

## 基板配置
![基板配置図](https://docs.google.com/drawings/d/e/2PACX-1vQ1sgteJVFo-8biVL6pwroS8nPYRfAhdPN9qbp7i--ZKk9LDnXRunqEiVy0HK8jNidJ4K8VbCYmiTqk/pub?w=960&h=720)
