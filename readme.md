# Latm_decorder
LATM/LOASのヘッダーを元にADTSなAACに変換します．  
同期層にLOASが使われている場合のみの対応です.　　

## 簡単な使い方
latm_decoder input.latm　　
## 仕様
ADTSの規格外である音声を正しく処理することは出来ません．
(ex. 7.1chを超えるチャンネルレイアウト)

## 制限・制約事項
LATM/LOASのCRC計算を行っていません．  
またARIB STD B32の制約条件を元に実装していますが,V-Lowマルチメディア放送向けの実装は含まれていません.

## ToDo
LATM/LOASデコード時のCRC計算  
ADTS出力時のCRC有無  
22.2ch向けに対するraw出力
