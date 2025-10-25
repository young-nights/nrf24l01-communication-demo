<style>
.highlight{
  color: white;
  background: linear-gradient(90deg, #ff6b6b, #4ecdc4);
  padding: 5px;
  border-radius: 5px;
}

.mint_green{
  color: white;
  background: #adcdadf2; 
  padding: 5px;
  border-radius: 5px;
}

.red {
  color: #ff0000;
}
.green {
  color:rgb(10, 162, 10);
}
.blue {
  color:rgb(17, 0, 255);
}

.wathet {
  color:rgb(0, 132, 255);
}
</style>




# <font size=3>一、工程介绍</font>
<font size=2>

这个项目工程一共包含两个代码，一个是`nRF24L01-Device-Receiver`，另一个是`nRF24L01-Device-Transmitter`，该仓库下目前有以下分支：

```bash
1. main
2. communication
```

其中，`communication`分支，主要用于测试两个nRF24L01的驱动代码以及参数配置匹配后，是否可以正常通讯，即最基础的底层驱动代码。

</font>

# <font size=3>二、nRF24L01使用规划</font>
<font size=2>在此次项目设计中，区别于`communication`分支，Receiver和Transmitter均新增接收解码函数以及数据包组帧发送函数，Receiver可以接收Transmitter发送的数据包，并根据相应的命令帧来执行特定动作。

## <font size=2>1. 收发数据规则</font>
在nRF24L01里，"多机通信≠以太网那种任意节点互发"，芯片硬件规则只有6条接收管道(pipe0 ~ pipe5)，且每条管道必须唯一匹配对方的发送地址。
因此，为了实现三个设备同时在线，任意两人能喊话，必须满足以下三个条件：
1. 每个节点用**唯一的发送地址**
2. 每个节点把所有可能喊它的地址写进自己的 RX 管道
3. 管道 2~5 只能存 1 字节差异（高 4 字节＝ Pipe1 的高 4 字节）

==注意1：对于普通的数据包发送，只要tx_addr的5字节地址与pipe0~pipe5管道的任意rx_addr地址完全一样，就可以正常的接发。==

==注意2：对于带ACK的数据包，nRF24L01接收到另一个发送过来的数据包后，回应ACK时必须把当前nRF24L01设备的tx_addr地址作为对方的目标rx_addr的地址，也即，对方必须把pipe0管道的接收地址设置为回应端tx_addr一样的地址，否则永远收不到ACK回应，也即认为发送失败。==

**总结：** 数据能走 P1～P5管道，ACK 只能走 P0管道，想让双向通信都成功，必须把“对方的发送地址”写进本机的 `RX_ADDR_P0`；P1～P5 只能用来收 “不需要 ACK” 或 “单向广播”数据.

**管道接收地址配置规则**
还需要注意的是，在nRF24L01中，`pipe0~pipe5`的`rx_addr`如果全部配置成同一个5字节地址，在接收阶段，nRF24L01空中出现包匹配时，6条管道均会同时命中，但nRF24L01硬件规定接收管道优先级：`P0 > P1 > P2 > P3 > P4 > P5`,其结果就是，状态寄存器 `STATUS.RX_P_NO` 总是给出 `000b（P0）`,只有 Pipe0 的 FIFO 会真正把这包数据压进去；其余管道虽然“命中”，但不会重复存同一包。在用户读取 payload 时，也只能从 Pipe0 的 FIFO 中读出一次。


</font>

<font size=3>详细关系图见下图：</font>
<br>
![流程图](./images/nRF24L01.svg)



## <font size=2>2. nRF24L01硬件连接说明</font>
<font size=2>nRF24L01采用SPI通讯方式进行控制，本项目基于RT-Thread SPI构架进行nRF24L01模组的参数配置，进而实现多设备的无线模组通信。

| MCU引脚 | nRF24L01引脚 | 配置说明                                                     |
| ------- | ------------ | ------------------------------------------------------------ |
| PB3     | SCK          | 通过CubeMX配置挂载                                           |
| PB4     | MISO         | 通过CubeMX配置挂载                                           |
| PB5     | MOSI         | 通过CubeMX配置挂载                                           |
| PA15    | CSN          | 配置为软件NSS，并使用```rt_hw_spi_device_attach```挂载到总线 |
| PB10    | IRQ          | 配置为外部中断，上拉输入，下降沿触发中断                     |
| PB11    | CE           | 配置为普通GPIO模式                                           |

</font>

## <font size=2>3. nRF24L01参数配置说明</font>

<font size=2>

| 寄存器 / 逻辑组   | 字段名（代码）     | 本次取值                     | 说明（作用与影响）                |
| ----------------- | ------------------ | ---------------------------- | --------------------------------- |
| **CONFIG**        | `prim_rx`          | `ROLE_PTX`                   | 上电后默认为 **主发送模式**       |
|                   | `pwr_up`           | 1                            | 直接进 Normal 模式                |
|                   | `crco`             | `CRC_2_BYTE`                 | CRC 长度 = 2 字节                 |
|                   | `en_crc`           | 1                            | 硬件 CRC 校验使能                 |
|                   | `mask_max_rt`      | 0                            | 重发超时不屏蔽 IRQ                |
|                   | `mask_tx_ds`       | 0                            | 发送完成不屏蔽 IRQ                |
|                   | `mask_rx_dr`       | 0                            | 接收完成不屏蔽 IRQ                |
| **EN_AA**         | `ENAA_P0`          | 1                            | pipe0 开启自动应答                |
|                   | `ENAA_P1`          | 1                            | pipe1 开启自动应答                |
|                   | `ENAA_P2`          | 1                            | pipe2 开启自动应答                |
|                   | `ENAA_P3`          | 0                            | pipe3 关闭自动应答                |
|                   | `ENAA_P4`          | 0                            | pipe4 关闭自动应答                |
|                   | `ENAA_P5`          | 0                            | pipe5 关闭自动应答                |
| **EN_RXADDR**     | `EN_RXADDR_P0`     | 1                            | pipe0 使能接收                    |
|                   | `EN_RXADDR_P1`     | 1                            | pipe1 使能接收                    |
|                   | `EN_RXADDR_P2`     | 1                            | pipe2 使能接收                    |
|                   | `EN_RXADDR_P3`     | 0                            | pipe3 失能接收                    |
|                   | `EN_RXADDR_P4`     | 0                            | pipe4 失能接收                    |
|                   | `EN_RXADDR_P5`     | 0                            | pipe5 失能接收                    |
| **SETUP_AW**      | `aw`               | 3                            | 地址宽度 = 5 字节                 |
| **SETUP_RETR**    | `arc`              | 15                           | 自动重发次数 = 15 次              |
|                   | `ard`              | `ADR_2Mbps`                  | 重发间隔 250 µs                   |
| **RF_CH**         | `rf_ch`            | 100                          | 载波频率 = 2400 + 100 = 2500 MHz  |
| **RF_SETUP**      | `rf_pwr`           | `RF_POWER_0dBm`              | 发射功率 0 dBm                    |
|                   | `rf_dr_low / high` | 0 / 0                        | 数据率 = 1 Mbps                   |
|                   | `cont_wave`        | 0                            | 关闭连续载波测试                  |
| **DYNPD**         | `DYNPD_P0`         | 1                            | pipe0 开启动态载荷                |
|                   | `DYNPD_P1`         | 1                            | pipe1 开启动态载荷                |
|                   | `DYNPD_P2`         | 1                            | pipe2 开启动态载荷                |
|                   | `DYNPD_P3`         | 0                            | pipe3 关闭动态载荷                |
|                   | `DYNPD_P4`         | 0                            | pipe4 关闭动态载荷                |
|                   | `DYNPD_P5`         | 0                            | pipe5 关闭动态载荷                |
| **FEATURE**       | `en_dpl`           | 1                            | 动态载荷总开关                    |
|                   | `en_ack_pay`       | 1                            | 允许 ACK 带数据包                 |
|                   | `en_dyn_ack`       | 1                            | 支持无 ACK 的数据发送             |
| **TX_ADDR**       | `txaddr[5]`        | `{0x55,0x0A,0x01,0x89,0x03}` | 本节点 **发送地址**               |
| **RX_ADDR_P0**    | `rx_addr_p0[5]`    | `{0x55,0x0A,0x01,0x89,0x01}` | 与 TX_ADDR 配对，用于收 ACK       |
| **RX_ADDR_P1**    | `rx_addr_p1[5]`    | `{0x55,0x0A,0x01,0x89,0x02}` | 多机通信时第 1 路接收地址         |
| **RX_ADDR_P2…P5** | 单字节             | 6,7,8,9                      | 仅低字节可变，高 4 字节与 P1 相同 |

</font>

## <font size=2>4. nRF24L01的待机模式</font>
<font size=2> 在 nRF24L01 里 `Standby-I` 和 `Standby-II` 不是单纯依靠“写寄存器”打开的，而是由 CE 引脚电平 + 内部状态机自动切换 的两种 上电待机子状态。只要 PWR_UP = 1（已上电），芯片就根据 CE 高低进入对应待机态。

| 状态       | 进入条件                             | 配置说明                                   |
| ---------- | ------------------------------------ | ------------------------------------------ |
| Standby-I  | PWR_UP = 1 且 CE = 0                 | 时钟运行，可读写寄存器/FIFO，不能发射/接收 |
| Standby-II | PWR_UP = 1 且 CE = 1 但 TX FIFO 为空 | 准备发射，一写 FIFO 就立即进入 TX 模式     |
| Power-Down | PWR_UP = 0                           | 晶振停振，SPI 仍可用，最省电               |

==注意：Standby-II 仅 PTX 模式存在；PRX 模式下 CE = 1 直接进入 RX 模式。==
</font>



# <font size=3>三、通讯协议</font>

<div style="background:#e8f5e8;padding:10px;border-radius:6px;color:#333;">

<font size=2> 

**1.版本说明**

| 版本号 | 变更原因 |
| ------ | -------- |
| V1.0   | 首次编制 |

**2.术语**

Receiver：接收方
Transmitter：发送方

**3.概述**

通信以主/从方式为主，Transmitter 作为通信主机，在需要时，发起一次通信。 Receiver 作为从机设备，收到主机命令后，执行相关动作或回应相关数据。当出现异常或其他情况时，Receiver 也会主动上报一些信息给 Transmitter 主机。

**4.串口参数**
==这个串口参数配置在使用串口通讯时起效，这里只是采用了帧结构。==

| 波特率  | 数据位 | 奇偶校验 | 停止位 |
| ------- | ------ | -------- | ------ |
| 9600bps | 8位    | 无       | 1位    |

**5.帧结构**

![帧结构图](./images/nRF24L01_Pic2.png)

<u> **帧头(Head)** </u>
固定为 0x55，0xAA

<u> **数据长度(Length)** </u>
设备编号+命令的字节数。即：(ID + Type + State + Code + Parameter)的字节数。

<u> **设备编号(ID)** </u>
设备机型对应的 ID 编码。<span class="red">例如：本次设备机型的 ID 编码为：0x00,0x73。</span>

<u> **命令类型(Type)** </u>
命令类型包含三类：
0x31: 命令（动作）类，从机收到此类命令，通常执行某些动作。
0x32: 写数据（设置参数）类，从机收到此类命令，更新相关参数，如模式、强度档位等。
0x33: 读数据（读取参数、状态）类，从机收到此类命令，回应相关参数或状态信息。
0x66: 主动上报（异常）类，比如温度过高、NTC 异常、强度改变等。

<u> **命令状态(State)** </u>
用于指示传输/校验是否出错。 Receiver 下发时，此域填 0x02。
Transmitter 回传时，若传输/校验出错，此域填 0x00；否则填 0x01。

<u> **命令编码(Code)** </u>
不同机型，根据具体功能，各自定义具体的命令码，详见机型功能命令码表。

<u> **参数列表(Parameter)** </u>
不同命令码，可以具有不同长度的参数，多字节参数时，高位字节在前。详见机型功能命令码表。

<u> **校验码(CRC)** </u>
校验多项式：X16+X15+X2+1。除帧头外，其它字节全部参与校验。校验码高位在前，低位在后。

</font>

</div>

# <font size=3>四、测试硬件</font>

<font size=2> 
本仓库的测试代码主要基于两个demo板，使用的nRF24L01是AS01系列的无线通讯模块，模板作为插件，插在功能板上，同样的，单片机最小系统板也插在功能板上。

![实物图](./images/nRF24L01_Pic1.png)

</font>


