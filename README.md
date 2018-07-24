# GR-Boards_NonContactMouse
GR-PEACH、および、GR-LYCHEEで動作するサンプルプログラムです。  
本サンプルはMbedオンラインコンパイラでは使用できません。  
OpenCVライブラリがGCC_ARMコンパイラにのみ対応しているため、Mbed CLI環境などでコンパイラにGCC_ARMを指定してご利用ください。  
GR-LYCHEEの開発環境については、[GR-LYCHEE用オフライン開発環境の手順](https://developer.mbed.org/users/dkato/notebook/offline-development-lychee-langja/)を参照ください。


## 概要
カメラを使った非接触USBマウスのサンプルです。OpenCVの optical flow を使い、カメラに写った物体の動きをベクトルで表します。各ベクトルの平均値から、マウスポインタの移動距離を決定します。

カメラの指定を行う場合(GR-PEACHのみ)は``mbed_app.json``に``camera-type``を追加してください。
```json
{
    "config": {
        "camera":{
            "help": "0:disable 1:enable",
            "value": "1"
        },
        "camera-type":{
            "help": "Please see mbed-gr-libs/README.md",
            "value": "CAMERA_CVBS"
        },
        "lcd":{
            "help": "0:disable 1:enable",
            "value": "0"
        }
    }
}
```

| camera-type "value"     | 説明                               |
|:------------------------|:-----------------------------------|
| CAMERA_CVBS             | GR-PEACH NTSC信号                  |
| CAMERA_MT9V111          | GR-PEACH MT9V111                   |
| CAMERA_OV7725           | GR-LYHCEE 付属カメラ               |
| CAMERA_OV5642           | GR-PEACH OV5642                    |
| CAMERA_WIRELESS_CAMERA  | GR-PEACH Wireless/Cameraシールド (OV7725) |

camera-typeを指定しない場合は以下の設定となります。  
* GR-PEACH、カメラ：CAMERA_MT9V111  
* GR-LYCHEE、カメラ：CAMERA_OV7725  

***Mbed CLI以外の環境で使用する場合***  
Mbed以外の環境(CLI or Mbedオンラインコンパイラ 以外の環境)をお使いの場合、``mbed_app.json``の変更は反映されません。  
``mbed_config.h``に以下のようにマクロを追加してください。  
```cpp
#define MBED_CONF_APP_CAMERA                        1    // set by application
#define MBED_CONF_APP_CAMERA_TYPE                   CAMERA_CVBS             // set by application
#define MBED_CONF_APP_LCD                           0    // set by application
```
