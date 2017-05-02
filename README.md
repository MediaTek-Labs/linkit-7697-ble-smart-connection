# Linkit 7697 BLE smart connection

How to use BLE to do smart connection on LinkIt 7697 HDK

Additinal resource can be found at https://docs.labs.mediatek.com/resource/mt7687-mt7697

## Folder Structure

* `android/ble_smart_connection`: Android studio project files.
* `project/linkit7697_hdk/apps/ble_smart_connect`: LinkIt SDK project files.

## How to Build

### Device Side

* Put/Extract the files into SDK root, so that there is `[SDK_root]/project/linkit7697_hdk/apps/ble_smart_connect`
* Execute `./build.sh linkit7697_hdk ble_smart_connect` under Linux enviornment
* Check generated bin at `[SDK_root]/out/linkit7697_hdk/ble_smart_connect/ble_smart_connect.bin`
* Use Flashtool to download `[SDK_root]/out/linkit7697_hdk/ble_smart_connect/flash_download.ini` into LinkIt 7697 HDK

### Mobile Side

* Extract `android/ble_smart_connection` to anywhere you prefer
* Have Android studio open the project
* Open `DeviceControlActivity.java`, modify variable `_ssid` and `_pwd` to fit your wireless enviornment.
* Build->Make Project
* Run->Run Application

## How to Run

### Phaese

* A. Turn on Device, when it is ready, it will start "BLE Advertising" (appear as BLE_SMTCN)
* B. Launch Android App, Scan and connects to BLE_SMTCN Device. After connected mobile will automatically send pre-defined Wi-Fi information to Device via BLE
* C. When Device receive the data, it will connect to Wi-Fi network and report back the SSID and IP to Mobile

### Mobile Side

![Mobile](/images/mobile_side.jpg)

### Device Side

Below are log output from UART port of LinkIt 7697 HDK
![Device_A](/images/device_side_A.jpg)
![Device_BC](/images/device_side_B_C.jpg)

## Message Sequence Chart

![MSC](/images/msc.png)


