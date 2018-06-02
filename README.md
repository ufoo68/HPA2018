# 操舵関係
## ハードウェア構成
- Arduino nano
- LANコネクタモジュール
- タクトスイッチ
- サーボモータ
- ジョイスティック
## ケーブルの割り当て
### 略語
e : エレベータ  
r : ラダー  
### LANケーブル left
緑白 : r left  
緑 : r neutral  
橙白 : r right  
青 : e down  
青白 : e neutral  
橙 : e up  
茶白 : none  
茶 : GND  

### LANケーブル right
緑白 : e servo  
緑 : r servo  
橙白 : none  
青 : none  
青白 : 5v  
橙 : 5v  
茶白 : GND  
茶 : GND  

### 信号線
黄 : r  
緑 : e  

### 基板配置
![基板配置図](https://docs.google.com/drawings/d/e/2PACX-1vQ1sgteJVFo-8biVL6pwroS8nPYRfAhdPN9qbp7i--ZKk9LDnXRunqEiVy0HK8jNidJ4K8VbCYmiTqk/pub?w=960&h=720)
