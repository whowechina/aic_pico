# AIC Pico，AIC Key 和 AIC Touch
  **AIME 读卡器 & Bandai Namco 读卡器 & Cardio 模拟器**

[Click here for the English version of this guide.](README.md)

<img src="doc/main.gif" width="90%">


为了省事，本文档是我从原先英文版翻译回中文的，高度依赖了 GitHub Copilot (GPT)，所以语气可能怪怪的，见谅。

**特性：**
* 它很小，据我所知是最小的。
* 有许多变种：
  * AIC Pico (PN532)
  * AIC Pico (PN5180, 无壳)
  * AIC Key PN532/PN5180
  * AIC Touch PN532/PN5180
  * AIC Pico Lib (参见下面的注释 1)
* 易于制作。
* Sega AIME I/O, Bandai Namco I/O 和 Spicetools CardIO 模拟。
* 自动检测上述 I/O 协议。
* 用于参数配置的命令行。
* 支持的卡：
  * FeliCa (Amusement IC)
  * ISO/IEC 14443 类型 A (BanaPassport, Mifare, Amiibo, 一些 IC Tag 和 ID Tag 卡, 等等.)
  * ISO/IEC 15693 (旧的 E-Amusement 卡)，仅 PN5180 支持 (参见下面的注释 2)
* 从任何 Mifare 卡模拟虚拟 AIC（暂不支持 Bandai Namco I/O 外）。
* 所有源文件开放。

**注释：**
1. 这样就可以把 "AIC Pico" 集成到基于 Raspberry Pi Pico 的其他项目中。可以参考我的 Chu Pico 项目里的用法。  
  <img src="doc/aic_pico_lib.jpg" width="25%">

2. PN532 只支持 14443A (Mifare) 和 FeliCa 卡，而 PN5180 还支持 15693 卡（旧的 e-Amusement 卡）。

## 感谢
感谢许多尊敬的爱好者和公司将他们的工具或材料免费或开源（KiCad, OnShape, InkScape, Fritzing, Raspberry things），ChatGPT 和 GitHub Copilot 也提供了很大的帮助。

同时感谢对我有很大帮助的社区开发者和项目：
* CrazyRedMachine (https://github.com/CrazyRedMachine) 提供了 Spicetools Card IO 部分；
* Sucareto 的 AIME Reader (https://github.com/Sucareto/Arduino-Aime-Reader) 提供了 AIME 协议部分；
* Bottersnike (https://gitea.tendokyu.moe/Bottersnike, https://sega.bsnk.me/) 提供了 AIME 和 FeliCa 相关的知识；
* .NET nanoFramework (https://github.com/nanoframework) 提供了 PN5180 部分；
* Gyt4 (https://github.com/gyt4/) 提供了 Bandai Namco 读卡器相关信息；
* Bananatools (https://gitea.tendokyu.moe/Hay1tsme/bananatools) 提供了 Bandai Namco 读卡器交互信息；
* chujohiroto (https://github.com/chujohiroto/Raspberry-RCS620S/blob/master/rcs620s.py) RCS620 用作间接参考；

## 友情警告
这个项目：
* 严重依赖于3D打印，Bambu AMS系统能够提供很大的帮助。
* 需要有焊接微小组件和线路的技能。

## 查看我的其他项目
你也可以查看我其他的酷炫项目。

<img src="https://github.com/whowechina/popn_pico/raw/main/doc/main.jpg" height="100px">
<img src="https://github.com/whowechina/iidx_pico/raw/main/doc/main.jpg" height="100px">
<img src="https://github.com/whowechina/iidx_teeny/raw/main/doc/main.jpg" height="100px">
<img src="https://github.com/whowechina/chu_pico/raw/main/doc/main.jpg" height="100px">
<img src="https://github.com/whowechina/mai_pico/raw/main/doc/main.jpg" height="100px">
<img src="https://github.com/whowechina/diva_pico/raw/main/doc/main.jpg" height="100px">
<img src="https://github.com/whowechina/aic_pico/raw/main/doc/main.gif" height="100px">
<img src="https://github.com/whowechina/groove_pico/raw/main/doc/main.gif" height="100px">

* Popn Pico: https://github.com/whowechina/popn_pico
* IIDX Pico: https://github.com/whowechina/iidx_pico
* IIDX Teeny: https://github.com/whowechina/iidx_teeny
* Chu Pico: https://github.com/whowechina/chu_pico
* Mai Pico: https://github.com/whowechina/mai_pico
* Diva Pico: https://github.com/whowechina/diva_pico
* AIC Pico: https://github.com/whowechina/aic_pico
* Groove Pico: https://github.com/whowechina/groove_pico

## **声明** ##
我在个人时间内制作了这个项目，没有任何经济利益或赞助。我将继续改进这个项目。我已尽我所能确保所有内容的准确性和功能性，但总有可能出现错误。如果你因使用这个开源项目而造成时间或金钱的损失，我不能负责。感谢你的理解。

查看我 GitHub 主页，上面有很多其他项目。
https://github.com/whowechina/

## 关于许可证
它是 CC-NC 授权。所以你只能给自己和你的朋友 DIY，不能利用这个项目赚钱。

## 多种制作版本选择
- [AIC Pico PN532](#制作-AIC-Pico-PN532)
- [AIC Pico PN5180](#制作-AIC-Pico-PN5180)
- [AIC Key](#制作-AIC-Key)
- [AIC Touch](#制作-AIC-Touch)

## 制作 AIC Pico PN532
说真的，这是我所有 Pico 系列项目中最简单的一个。
### 组件
* 1x 树莓派 Pico 或 Pico W (克隆版也可以)。  
  https://www.raspberrypi.com/products/raspberry-pi-pico
* 1x PN532 模块 (红色方形板版本，便宜的克隆版也可以)。  
  https://www.elechouse.com/product/pn532-nfc-rfid-module-v4/
* 一些细一点的电线，软一点的硅胶线更好。
* 小巧的 WS2812B LED 灯条。
* 4x M2*8mm 螺丝。

### 3D 打印
* **aic_pico_bottom.stl**  
  底部部分。

对于顶部部分，选择适合你需求的一个。
* **aic_pico_top.stl**  
  常规顶部部分。
* **aic_pico_top_ams.3mf**  
  常规顶部部分，多色打印。
* **aic_pico_top_tall.stl**  
  加高的顶部部分，可以适应更厚的 LED 条。
* **aic_pico_top_tall_ams.3mf**  
  加高的顶部部分，多色打印。

顶部部分需要上下颠倒打印。

### 接线
<img src="doc/pico_pn532_wiring.png" width="70%">

### 组装
我会让这些图片来说明。

<img src="doc/pico_assemble_1.jpg" width="40%">
<img src="doc/pico_assemble_2.jpg" width="34%">
<img src="doc/pico_assemble_3.jpg" width="37%">
<img src="doc/pico_assemble_4.jpg" width="40%">

#### 注意事项
* 固件支持最多 64 个 LED 的 WS2812B 灯条。我用了 3 个，如主标题图像所示。但你可以用不同数量的 LED，只要它们能装进壳子里。
* 即使在低设置下，LED 可能过于明亮，考虑用一些胶带覆盖它。
* PN532 上的模式开关必须处于 "I2C" 模式，下面的图片显示了正确的设置。  
  <img src="doc/pn532_i2c.jpg" width="40%">

## 制作 AIC Pico PN5180
如果你选择 PN5180 NFC 模块，壳子的设计要你自己来了，确保它适合你的要求就好，或者你可以不使用壳子。与 PN532 版本相比，准备好焊接更多的线。

### 接线
<img src="doc/pico_pn5180_wiring.png" width="70%">

注意：WS2812B LED 条的接线与 PN532 版本相同。

## 制作 AIC Key
AIC Key 是 AIC Pico 的一个变种 - 集成了一个小键盘。比 "AIC Pico" 更难构建，因为它有许多微小的组件需要焊接。

### 组件
* 1x 树莓派 Pico 或 Pico W (克隆版也可以)。  
  https://www.raspberrypi.com/products/raspberry-pi-pico

* 对于 NFC 模块，选择以下选项之一：
  * 1x PN532 模块 (红色方形板版本，便宜的克隆版也可以)。  
    https://www.elechouse.com/product/pn532-nfc-rfid-module-v4/
  * 1x PN5180 模块 (蓝色矩形版本，便宜的克隆版也可以)。PN5180 支持 ISO/IEC 15693 (旧的 e-amusement 卡)。  
    <img src="doc/pn5180.jpg" width="50%">

* 对于 LED，你有 2 个选项：
  * 选项 1：6x 侧光 WS2812B 1204 LED（D1 到 D6）和一个 10ohm 0603 电阻（R1），在组装图像中标记为绿色。( AIC Key 和 AIC Touch 均支持)
  * 选项 2：6x 常规单色 0603 LED（D7 到 D12）和 6x 100ohm 0603 电阻（R2 到 R7），在组装图像中标记为紫色。(仅 AIC Key)

* C1, C2, C3 电容, 0603 0.1uF。它们提供更好的供电稳定性。

* 对于开关，你有 2 个选项：
  * 选项 1：12x ALPS SKRRAAE010 超薄轻触 开关。  
    https://www.mouser.com/ProductDetail/Alps-Alpine/SKRRAAE010?qs=m0BA540hBPeKhAe3239t1w%3D%3D
  * 选项 2：12x Panasonic EVQP1K05M 6mm 方形触摸开关。  
    https://www3.panasonic.biz/ac/e/dl/catalog/index.jsp?series_cd=3473&part_no=EVQP1K05M

* 1x 右角 2.54mm 间距头，PN532 为 4P，PN5180 为 13P。  
  <img src="doc/right_angle_header.jpg" width="20%">

* 1x 数字贴纸。你可以找一些定制贴纸服务，或者你可以找一些现有的贴纸。我发现这个 Bonito 水晶 3D 贴纸对这个项目非常有帮助。  
  <img src="doc/bonito_stickers.png" width="50%">  
  <img src="doc/bonito_action.jpg" width="50%">

* PCB，访问 JLCPCB (https://jlcpcb.com/) 并在那里下订单。保持所有设置默认，1.6mm 厚度，你喜欢的任何颜色。PCB gerber 文件在 "Production/PCB/aic_key_v*.zip"。  
  <img src="doc/pcbs.jpg" width="60%">

### 3D 打印
* **aic_key_bottom.stl**  
  底部部分。
* **aic_key_top_surface.stl**  
  适用于 Alps 表面开关的顶部部分。
* **aic_key_top_surface_ams.3mf**  
  适用于 Alps 表面开关的顶部部分，多色打印。
* **aic_key_top_tact.stl.stl**  
  适用于 Panasonic tact 开关的顶部部分。
* **aic_key_top_tact_ams.3mf**  
  适用于 Panasonic tact 开关的顶部部分，多色打印。

顶部部分需要上下颠倒打印。

### 组装
我还是让这些图片来说明。记住在组装之前先将固件上传到树莓派 Pico。
注意，最新的 PCB 已经同时兼容 PN532 和 PN5180，根据需要焊接即可。下面的照片是基于早期的 PCB 版本拍摄的，不能精确对应，但我相信你能够看明白。

#### 通用部分
<img src="doc/key_assemble_1a.jpg" width="30%">
<img src="doc/key_assemble_1b.jpg" width="30%">
<img src="doc/key_assemble_2.jpg" width="30%">

#### PN532 版本
记住首先设置为 I2C 模式。

<img src="doc/pn532_i2c.jpg" width="40%">
<img src="doc/key_assemble_3.jpg" width="80%">
<img src="doc/key_assemble_4.jpg" width="40%">
<img src="doc/key_assemble_5.jpg" width="40%">

**注意：**
- PN532 PCB 上的天线有可能会接触到 USB 插座并导致射频问题。为了防止这种情况，你可以贴上绝缘胶带避免它们互相接触。

<img src="doc/key_pn532_tape.jpg" width="60%">
- 由于制造误差，您手上的 PN532 模块可能无法完美地适应外壳。如有必要，您可以略微修剪 PN532 模块的边缘来调整。

#### PN5180 版本
你需要切掉原来的天线并使用我们 PCB 中的天线。

<img src="doc/key_assemble_6.jpg" width="40%">
<img src="doc/key_assemble_7.jpg" width="40%">
<img src="doc/key_assemble_8.jpg" width="40%">
<img src="doc/key_assemble_9.jpg" width="40%">

#### Final Assembly
你可以使用一些强力胶水来固定贴纸。

<img src="doc/key_assemble_10.jpg" width="46%">
<img src="doc/pico_assemble_4.jpg" width="40%">

## 制作 AIC Touch
AIC Touch 是 AIC Pico 的另一种版本，配备了触摸屏。不过它微小的 FPC/FFC 连接器大大增加了焊接和组装的难度。

### 组件
* 对于 Raspberry Pi Pico、NFC 模块、6x WS2812B 1204 LED 和 R1 电阻部分，请按照 AIC Key 的指南进行操作。
* C1、C2、C3，0603 0.1uF 电容，它们有助于更稳定的电源供应。
* R2、R3，I2C 总线的上拉电阻，0603 1Kohm ~ 4.7Kohm。
* 1.69 英寸 240x280 LCD 触摸屏（ST7789 + CST816）。有几个供应商在销售这个型号，只要看起来完全相同，就应该可以。它看上去长这个样子。

  <img src="doc/touchscreen.jpg" width="30%">

* 触摸屏的 FPC/FFC 翻盖连接器。18P，0.5mm 间距，1.0H（1mm 高度），前翻盖下接。它看起来像这样：

  <img src="doc/fpc_socket.jpg" width="30%">

* PCB 文件 "aic_touch_v*.zip"。  
  <img src="doc/pcbs.jpg" width="60%">

### 3D 打印
* **aic_touch_bottom.stl**  
  底部部分。
* **aic_touch_top.stl**  
  顶部部分.
* **aic_touch_top_ams.3mf**  
  顶部部分，多色打印。

顶部部分需要上下颠倒打印。

为了获得最佳的外观效果，我建议使用透明 PLA 和黑色 PLA 的组合。其中顶部除了子对象“Top”的最开始两层用黑色外，其他都用透明。    
  <img src="doc/touch_3d_print.jpg" width="40%">

### 组装
对于大部分内容，请参考 AIC Key 的制作。

* 触摸屏会严重阻挡 PN532 的 RF 信号，所以我们需要将天线转移到 AIC Touch 的 PCB 上，就像前面 PN5180 版本那样。
  * 制作 PN5180 版本时需要短接 "PN532 ANT"（图片中的绿色标记）。制作 PN532 版本时则需要短接 "PN5180 ANT"（图片中的红色标记）。    
    <img src="doc/ant_short.png" width="60%">
  * 和 PN5180 不同，PN532 有 3 个天线引脚。要把 PN532 的天线转移到我们 AIC Touch 的 PCB 上，先用小刀或凿子切断原来的 2 条天线走线（图片中的绿色标记），然后将 3 个 "PN532 ANT" 引脚连接到 PN532 模块对应位置。    
    <img src="doc/pn532_ant.png" width="60%">

* 焊接 18P-0.5mm-1.0H FPC/FFC 连接器很有挑战。以下是一些提示：
  * 使用小而尖锐的烙铁头，越小越好。
  * 烙铁头温度设定在较低的温度，比如大约 280 摄氏度。
  * 使用大量的高质量助焊剂，以保持引脚在整个焊接过程中“湿润”。
  * 首先焊接两侧的固定引脚以确保对齐，然后再焊接 18 个主引脚。
  * 避免直接将焊锡丝喂到主引脚来焊接。
  * 在焊接主引脚时，只需要在烙铁尖上附着极少量的焊锡。
  * 如果你用了过多的焊锡，可以用吸锡带去除多余的焊锡。

* 记得贴一层 1mm 厚度的软垫以支撑触摸屏。你可以用 1mm 厚度的 3M VHB 双面胶带，但是不要移除红色的保护层（绝对不要让胶带粘在触摸屏上）。

  <img src="doc/touch_assemble_1.jpg" width="30%">
  <img src="doc/touch_assemble_2.jpg" width="30%">

* 搞定！

  <img src="doc/touch_assemble_3.jpg" width="30%">

## 固件
* UF2 文件在 `Production\Firmware` 文件夹中。
* 有几种方法可以进入固件更新模式：
  * 对于新构建，握住 BOOTSEL 按钮，同时将 USB 连接到 PC，会出现一个名为 "RPI-RP2" 的磁盘。将 UF2 固件二进制文件拖入其中。这样就 OK 了。
  * 如果已经上传过可用的的固件，你可以在命令行中使用 "update" 命令来更新未来的固件，这样你就不需要打开外壳。
  * 如果已经有一个在 2023-12-02 之后的工作固件，你也可以在插入 USB 线时同时按下 "00" 键和 "·" 键（或直接接地 GPIO10 和 GPIO11），它将进入固件更新模式。
* 其中一个 USB 串口是命令行，你可以使用这个 Web Serial Terminal 来连接和访问命令行。（注意："?" 是帮助）  
  https://googlechromelabs.github.io/serial-terminal/
* 另外一个串口上运行读卡器协议，当前支持 AIME 和 Bandai Namco。
* Spicetools cardio (Card I/O) HID 是一直开启的，除非读卡器协议正在工作中。
* 如果你的 PN5180 模块在读取 Mifare 卡（AIME 卡和 Bana Passport 卡）的时候遇到问题，你可以试试用“pn5180_tweak on”命令来启用 PN5180 射频调整。
* 一些命令行命令：
  * "light \<rgb|led|both|off\>" 来打开或关闭 LED。
  * "level <0..255> <0..255>" 来调整亮度。
  * "nfc" 手动检测卡片。
  * "update" 重启进入固件更新模式。
* 鉴于我的业余时间有限，固件可能没有完全测试。请报告任何异常。

## 卡的 ID 生成逻辑
为了支持许多不同的 NFC 卡和标签，卡 ID 按照以下规则进行转换。
### AIME
* 15693 => 0x01 + UID 的最后 7 个字节
* MIFARE (4-byte UID) => 0x01 + 0x01 + UID + UID 的前 2 个字节
* MIFARE (7-byte UID) => 0x01 + UID
* FeliCa => 原始 IDm
### CardIO
* 15693 => 原始 UID
* MIFARE (4-byte UID) => 0xE0 + 0x04 + UID + UID 的前 2 个字节
* MIFARE (7-byte UID) => 0xE0 + UID
* FeliCa => 原始 IDm
### Bandai Namco
* MIFARE (4-byte UID) => UID
* FeliCa => 原始 IDm

## 3D 模型源文件 (Onshape)
https://cad.onshape.com/documents/ca5497f91b2962105335e822/w/7b88022e98c02c60ad0c44a7/e/c3476efd13c08f807f3773fe?configuration=List_6ARRO0azcgmmHg%3D__&renderMode=1&rightPanel=configPanel&uiState=6558cabf9b380560ca5b554e
