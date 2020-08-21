Arduinoの USB/HID を使って文字列を出力しブラウザーと連携して、エアコンのON/OFFと温度調整制御をおこないます。ブラウザー側で制御値を生成し、それを音を使った下り通信で Arduino に送ります。 Arduino では、エアコンの赤外線リモコンの仕組みで、エアコンをコントロールします。
赤外線リコモンの送信データは、学習により Arduino 内部の仮想EEPROMに保管して利用するため、各種メーカに対応できます。

詳細はQiita参照https://qiita.com/pronechia/private/b9623c5b8ac65b95edbf (限定公開中)

注意）

・Windows用とMac用で、ソースを分離しました。環境に合わせて利用してください。

各自の環境にあわせて確認、修正する箇所があります。ソースコードの上部のコメントを参考に、対応してください。

・実行時の注意

WindowsPC、Macとも、入力モードは、英数字である必要があります。日本語モードになっていないことを確認してください。

This is an Arduino USB/HID with Tone sample program .
