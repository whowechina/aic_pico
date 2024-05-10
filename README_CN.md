# AIC Pico 和 AIC Key
  **AIME 读卡器 & Bandai Namco 读卡器 & Cardio 模拟器**

[Click here for the English version of this guide.](README.md)

<img src="doc/main.jpg" width="80%">


为了省事，本文档是我从原先英文版翻译回中文的，高度依赖了 GitHub Copilot (GPT)，所以语气可能怪怪的，见谅。

**特性：**
* 它很小，据我所知是最小的。
* 有许多变种：
  * AIC Pico (PN532)
  * AIC Pico (PN5180, 无壳)
  * AIC Key PN532
  * AIC Key PN5180
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

<img src="https://github.com/whowechina/popn_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/iidx_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/iidx_teeny/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/chu_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/mai_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/diva_pico/raw/main/doc/main.jpg" height="100px"><img src="https://github.com/whowechina/aic_pico/raw/main/doc/main.jpg" height="100px">

* Popn Pico: https://github.com/whowechina/popn_pico
* IIDX Pico: https://github.com/whowechina/iidx_pico
* IIDX Teeny: https://github.com/whowechina/iidx_teeny
* Chu Pico: https://github.com/whowechina/chu_pico
* Mai Pico: https://github.com/whowechina/mai_pico
* Diva Pico: https://github.com/whowechina/diva_pico
* AIC Pico: https://github.com/whowechina/aic_pico

## **声明** ##
我在个人时间内制作了这个项目，没有任何经济利益或赞助。我将继续改进这个项目。我已尽我所能确保所有内容的准确性和功能性，但总有可能出现错误。如果你因使用这个开源项目而造成时间或金钱的损失，我不能负责。感谢你的理解。

查看我 GitHub 主页，上面有很多其他项目。
https://github.com/whowechina/

## 关于许可证
它是 CC-NC 授权。所以你只能给自己和你的朋友 DIY，不能利用这个项目赚钱。

## 构建 "AIC Pico (PN532)"
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

## 构建 "AIC Pico (PN5180)"
如果你选择 PN5180 NFC 模块，壳子的设计要你自己来了，确保它适合你的要求就好，或者你可以不使用壳子。与 PN532 版本相比，准备好焊接更多的线。

### 接线
<img src="doc/pico_pn5180_wiring.png" width="70%">

注意：WS2812B LED 条的接线与 PN532 版本相同。

## 构建 "AIC Key"
AIC Key 是 AIC Pico 的一个变种 - 集成了一个小键盘。比 "AIC Pico" 更难构建，因为它有许多微小的组件需要焊接。

### 组件
* 1x 树莓派 Pico 或 Pico W (克隆版也可以)。  
  https://www.raspberrypi.com/products/raspberry-pi-pico

* 对于 NFC 模块，选择以下选项之一：
  * 1x PN532 模块 (红色方形板版本，便宜的克隆版也可以)。  
    https://www.elechouse.com/product/pn532-nfc-rfid-module-v4/
  * 1x PN5180 模块 (蓝色矩形版本，便宜的克隆版也可以)。PN5180 支持 ISO/IEC 15693 (旧的 e-amusement 卡)。  
    <img src="doc/pn5180.jpg" width="50%">

* 对于 LED，你有 3 个选项：
  * 选项 1：6x 侧光 WS2812B 1204 LED（D1 到 D6）和一个 10ohm 0603 电阻（R1），在组装图像中标记为绿色。
  * 选项 2（仅适用于 PN532 版本）：6x WS2812C-2020 LED（D13 到 D18），在组装图像中标记为蓝色。
  * 选项 3：6x 常规单色 0603 LED（D7 到 D12）和 6x 100ohm 0603 电阻（R2 到 R7），在组装图像中标记为紫色。

* 对于开关，你有 2 个选项：
  * 选项 1：12x ALPS SKRRAAE010 低调 TACT 开关。  
    https://www.mouser.com/ProductDetail/Alps-Alpine/SKRRAAE010?qs=m0BA540hBPeKhAe3239t1w%3D%3D
  * 选项 2：12x Panasonic EVQP1K05M 6mm 方形触摸开关。  
    https://www3.panasonic.biz/ac/e/dl/catalog/index.jsp?series_cd=3473&part_no=EVQP1K05M

* 1x 右角 2.54mm 间距头，PN532 为 4P，PN5180 为 13P。  
  <img src="doc/right_angle_header.jpg" width="20%">

* 1x 数字贴纸。你可以找一些定制贴纸服务，或者你可以找一些现有的贴纸。我发现这个 Bonito 水晶 3D 贴纸对这个项目非常有帮助。  
  <img src="doc/bonito_stickers.png" width="50%">  
  <img src="doc/bonito_action.jpg" width="50%">

* PCB，只需访问 JLCPCB (https://jlcpcb.com/) 并在那里下订单。保持所有设置默认，1.6mm 厚度，你喜欢的任何颜色。PCB gerber 文件在 "Production/PCB" 文件夹。对于 PN532 版本，使用 "aic_key_pn532_v*.zip"，对于 PN5180 版本，使用 "aic_key_pn5180_v*.zip"。  
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

### 组装
我还是让这些图片来说明。记住在组装之前先将固件上传到树莓派 Pico。

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

## 固件
* UF2 文件在 `Production\Firmware` 文件夹中。
* 有几种方法可以进入固件更新模式：
  * 对于新构建，握住 BOOTSEL 按钮，同时将 USB 连接到 PC，会出现一个名为 "RPI-RP2" 的磁盘。将 UF2 固件二进制文件拖入其中。这样就 OK 了。
  * 如果已经上传过可用的的固件，你可以在命令行中使用 "update" 命令来更新未来的固件，这样你就不需要打开外壳。
  * 如果已经有一个在 2023-12-02 之后的工作固件，你也可以在插入 USB 线时同时按下 "00" 键和 "·" 键（或直接接地 GPIO10 和 GPIO11），它将进入固件更新模式。
* 你可以使用这个 Web Serial Terminal 来连接到板的主 USB 串行端口来访问命令行。（注意："?" 是帮助）  
  https://googlechromelabs.github.io/serial-terminal/
* 支持 Spicetools cardio (Card I/O) HID。
* 在第二个串行端口上跑的是 SEGA AIME 协议。
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
